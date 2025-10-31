#ifndef INCLUDE_ARC_UNION_HPP
#define INCLUDE_ARC_UNION_HPP

#include "arc/context.hpp"
#include "arc/detail/cast.hpp"
#include "arc/detail/concepts.hpp"
#include "arc/detail/select.hpp"
#include "arc/detail/type_at.hpp"
#include "arc/detail/with_index.hpp"

#include "arc/alias.hpp"
#include "arc/cluster.hpp"
#include "arc/context_fwd.hpp"
#include "arc/defer.hpp"
#include "arc/factory.hpp"
#include "arc/finalise.hpp"
#include "arc/global_context.hpp"
#include "arc/global_trait.hpp"
#include "arc/key.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/node.hpp"
#include "arc/resolve.hpp"
#include "arc/trait.hpp"
#include "arc/traits.hpp"
#include "arc/traits/dynamic_node.hpp"
#include "arc/traits/dynamic_node.hxx"
#include "arc/traits/scheduler.hpp"
#include "arc/union_fwd.hpp"

#if !ARC_IMPORT_STD
#include <cstddef>
#include <new>
#include <type_traits>
#include <utility>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<IsNodeHandle... Options>
struct Union
{
    static constexpr auto withIndex = arc::withIndex<sizeof...(Options)>;

    template<class Context>
    class Node : public arc::Node
    {
        struct InnerContext : Context
        {
            static InnerContext isUnionContext(detail::IsUnionContextTag);

            template<IsNodeHandle T, class Option>
            static constexpr auto exchangeImpl(Option& option, auto&&... args)
            {
                static_assert(not std::is_const_v<Option>, "Cannot exchange implementation of a node in a const context");
                static_assert((... or std::is_same_v<T, Options>), "Cannot exchange with a node that is not listed as an option in the hosting arc::Union");
                auto const nodePtr = detail::memberPtr<Node>(std::bit_cast<Option Node::*>(&Node::bytes));
                Node& node = nodePtr.getClassFromMember(option);
                if constexpr (ContextHasGlobalTrait<Context, Global<trait::Scheduler>>)
                {
                    static_assert(ContextHasTrait<ContextOf<Option>, trait::DynamicNode>,
                        "Node inside arc::Union can only exchange its implementation with a scheduler when it has the arc::trait::DynamicNode trait");
                    auto const scheduler = node.getGlobal(trait::scheduler);
                    // Only execute the exchange if the scheduler is in exclusive mode as this is the
                    // way to guarantee there will be no dangling references to the old implementation
                    if (scheduler.inExclusiveMode())
                    {
                        return arc::Defer<>(
                            [&node, ...args = ARC_FWD(args)]() mutable
                            {
                                node.template emplace<T>(std::move(args)...);
                            });
                    }
                    else
                    {
                        scheduler.postExclusiveTask(
                            trait::DynamicNode::exchangeImpl{},
                            Context{}.template getParentNode<typename Context::Root::Context>(node),
                            [](auto& graph)
                            {
                                graph.visitTrait(
                                    trait::dynamicNode,
                                    []<class DN>(DN dynamicNode)
                                    {
                                        if constexpr (IsDynamicContext<ContextOf<DN>>)
                                        {
                                            dynamicNode.exchangeImpl();
                                        }
                                    });
                            }
                        );
                        return arc::Defer();
                    }
                }
                else
                {
                    // Take args by value in case they come from the current node's state, which will be invalidated during the call to emplace
                    return arc::Defer(
                        [&node, ...args = ARC_FWD(args)]() mutable
                        {
                            node.template emplace<T>(std::move(args)...);
                        });
                }
            }

            template<class Option, IsTrait Trait>
            requires detail::HasLink<Context, Trait> or detail::IsGlobalTrait<Trait>
            static constexpr auto getNode(Option& option, Trait trait)
            {
                using OptionNode = detail::SelectIf<
                    detail::NodeHasUnderlyingNodePred<detail::UnderlyingNode<Option>>,
                    ToNode<Options>...
                >;
                auto const nodePtr = detail::memberPtr<Node>(std::bit_cast<OptionNode Node::*>(&Node::bytes));
                return Context{}.getNode(nodePtr.getClassFromMember(option), trait);
            }

            constexpr auto& getGlobalNode(auto& node)
            {
                return Context{}.getGlobalNode(getParentNode<Context>(node));
            }

            template<IsContext Parent>
            constexpr auto getParentMemPtr()
            {
                // Disable member pointer access to children as the implementation can be swapped out at runtime
                static_assert(detail::alwaysFalse<Parent>, "arc::Union does not have a stable member pointer relative to its parent node");
            }

            template<IsContext Parent, class Option>
            constexpr auto& getParentNode(Option& node)
            {
                if constexpr (std::is_same_v<detail::Decompress<Parent>, InnerContext>)
                {
                    return node;
                }
                else
                {
                    using OptionNode = detail::SelectIf<
                    detail::NodeHasUnderlyingNodePred<detail::UnderlyingNode<Option>>,
                        ToNode<Options>...
                    >;
                    auto const nodePtr = detail::memberPtr<Node>(std::bit_cast<OptionNode Node::*>(&Node::bytes));
                    return Context{}.template getParentNode<Parent>(nodePtr.getClassFromMember(node));
                }
            }
        };

        template<class Option>
        using ToNode = detail::ToNodeState<typename ToNodeWrapper<Option>::template Node<detail::CompressContext<InnerContext>>>;

    public:
        template<std::size_t I>
        using NodeAt = ToNode<detail::TypeAt<I, Options...>>;

        static constexpr bool isUnary() { return (... and ToNode<Options>::isUnary()); }

        template<std::size_t I, class Trait>
        struct TypesAtT : detail::ResolveTrait<NodeAt<I>, Trait>::Types
        {
            static constexpr std::size_t TypesCount = sizeof...(Options);
            template<std::size_t UnionIndex>
            using TypesAt = TypesAtT<UnionIndex, Trait>;
        };

        template<class>
        struct AsTrait;

        template<class Trait>
        requires HasTrait<NodeAt<0>, Trait>
        using TraitsTemplate = arc::ResolvedTrait<AsTrait<Trait>, TypesAtT<0, Trait>>;

        using Traits = arc::TraitsTemplate<Node, TraitsTemplate>;

        template<std::invocable<Constructor<Node>> F>
        requires std::same_as<Node, std::invoke_result_t<F, Constructor<Node>>>
        explicit constexpr Node(WithFactory, F factory)
            : Node(factory(Constructor<Node>()))
        {}

        template<std::size_t I>
        requires (I < sizeof...(Options))
        explicit(false) constexpr Node(std::in_place_index_t<I>, auto&&... args)
            : index(I)
        {
            new (bytes) NodeAt<I>(ARC_FWD(args)...);
        }

        template<std::size_t I>
        requires (I < sizeof...(Options))
        constexpr NodeAt<I>& emplace(auto&&... args)
        {
            if constexpr (ContextHasGlobalTrait<Context, Global<trait::Scheduler>>)
            {
                if (not getGlobal(trait::scheduler).inExclusiveMode())
                    throw std::runtime_error("arc::Union::emplace can only be called when the scheduler is in exclusive mode");
            }
            destroy();
            index = I;
            NodeAt<I>& next = *new (bytes) NodeAt<I>(ARC_FWD(args)...);
            next.visit(detail::OnGraphConstructedVisitor{});
            return next;
        }

        template<class Option>
        requires (... || std::same_as<Option, Options>)
        explicit(false) constexpr Node(std::in_place_type_t<Option>, auto&&... args)
            : index(findIndex<Option>())
        {
            new (bytes) ToNode<Option>{ARC_FWD(args)...};
        }

        template<class Option>
        requires (... || std::same_as<Option, Options>)
        constexpr ToNode<Option>& emplace(auto&&... args)
        {
            if constexpr (ContextHasGlobalTrait<Context, Global<trait::Scheduler>>)
            {
                if (not getGlobal(trait::scheduler).inExclusiveMode())
                    throw std::runtime_error("arc::Union::emplace can only be called when the scheduler is in exclusive mode");
            }
            destroy();
            index = findIndex<Option>();
            ToNode<Option>& next = *new (bytes) ToNode<Option>{ARC_FWD(args)...};
            next.visit(detail::OnGraphConstructedVisitor{});
            return next;
        }

        template<class Visitor>
        constexpr decltype(auto) visit(this auto& self, Visitor&& visitor)
        {
            return self.withNode(
                [&](auto& node) -> decltype(auto)
                {
                    return node.visit(ARC_FWD(Visitor, visitor));
                });
        }

        template<std::size_t Index>
        constexpr auto* getIf(this auto& self)
        {
            return self.index == Index
                ? self.template get<Index>()
                : nullptr;
        }

        template<class Option>
        requires (... || std::same_as<Option, Options>)
        constexpr auto* getIf(this auto& self)
        {
            return self.template getIf<findIndex<Option>()>();
        }

        constexpr ~Node()
        {
            destroy();
        }

    private:
        template<std::size_t Index>
        constexpr auto* get() &
        {
            return std::launder(reinterpret_cast<NodeAt<Index>*>(bytes));
        }

        template<std::size_t Index>
        constexpr auto const* get() const &
        {
            return std::launder(reinterpret_cast<NodeAt<Index> const*>(bytes));
        }

        template<class F>
        constexpr decltype(auto) withNode(this auto& self, F&& f)
        {
            return withIndex(
                self.index,
                [&](auto i) -> decltype(auto)
                {
                    return std::invoke(ARC_FWD(F, f), *self.template get<i>());
                });
        }

        constexpr void destroy()
        {
            withNode([]<class T>(T& node) -> void { node.~T(); });
        }

        template<class Option>
        static consteval std::size_t findIndex()
        {
            std::size_t i = 0, c = 0;
            (((i |= std::is_same_v<Option, Options>, (c += i))), ...);
            if (c == 0)
                throw "Not a valid Option";
            return sizeof...(Options) - c;
        }

        static constexpr std::size_t Align = std::max({alignof(ToNode<Options>)...});
        static constexpr std::size_t Size = std::max({sizeof(ToNode<Options>)...});

        std::size_t index;
        alignas(Align) std::byte bytes[Size];
    };
};

template<IsNodeHandle... Options>
template<class Context>
template<class Trait>
struct Union<Options...>::Node<Context>::AsTrait : Node
{
    template<class Source, class Key = ContextOf<Source>::Info::DefaultKey>
    constexpr auto finalise(this auto& self, Source& source, Key const& key = {}, auto const&... keys)
    {
        // Don't consume the key, as it needs to be applied once we know the active option
        return arc::finalise<false>(source, self, key, keys...);
    }

    template<class Self, class... Args>
    constexpr decltype(auto) implWithKey(this Self& self, auto const& key, auto const& keys, Args&&... args)
    {
        using Environment = Self::Environment;
        return self.withNode(
            [&](auto& node) -> decltype(auto)
            {
                return std::apply(
                    [&](auto const&... ks) -> decltype(auto)
                    {
                        return withEnv<Environment>(node).asTrait(Trait{}, key, ks...).impl(ARC_FWD(Args, args)...);
                    },
                    keys);
            });
    }

    template<class Self>
    constexpr decltype(auto) impl(this Self& self, auto&&... args)
    {
        return self.implWithKey(typename ContextOf<Self>::Info::DefaultKey{}, std::tuple(), ARC_FWD(args)...);
    }
};

} // namespace arc

#endif // INCLUDE_ARC_UNION_HPP
