#ifndef INCLUDE_ARC_VIRTUAL_HPP
#define INCLUDE_ARC_VIRTUAL_HPP

#include "arc/detail/as_ref.hpp"
#include "arc/detail/select.hpp"
#include "arc/cluster.hpp"
#include "arc/empty_types.hpp"
#include "arc/factory.hpp"
#include "arc/global_context.hpp"
#include "arc/global_trait.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/node.hpp"
#include "arc/traits.hpp"
#include "arc/traits/dynamic_node.hxx"
#include "arc/virtual_fwd.hpp"
#include "arc/traits/dynamic_node.hpp"
#include "arc/traits/scheduler.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <memory>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>
#endif

namespace arc {

struct detail::INodeBase : Node
{
    template<IsNodeHandle T, class Self>
    requires IsVirtualContext<ContextOf<Self>>
    constexpr auto exchangeImpl(this Self& self, auto&&... args)
    {
        static_assert(not std::is_const_v<Self>, "Cannot exchange implementation of a node in a const context");
        return ContextOf<Self>::template exchangeImpl<T>(self, ARC_FWD(args)...);
    }

    constexpr auto* asInterface(this auto& self) { return std::addressof(self); }
};

ARC_MODULE_EXPORT
struct INode : virtual detail::INodeBase
{};

ARC_MODULE_EXPORT
template<IsInterface... Interfaces>
struct Virtual
{
    template<class Context>
    struct Node : arc::Node
    {
    private:
        struct ImplBase : IDestructible
        {
            constexpr ImplBase(Node* virtualHost)
                : virtualHost(virtualHost)
                , globalNode(virtualHost)
            {}

            virtual void setVirtualHost(Node* newVirtualHost)
            {
                virtualHost = newVirtualHost;
                globalNode.set(newVirtualHost);
            }
            Node* getVirtualHost() const { return virtualHost; }
            auto* getGlobalNode() const { return globalNode.get(); }

            virtual void onGraphConstructed() = 0;
            virtual void visitExchangeImpl() = 0;

        private:
            Node* virtualHost;
            [[no_unique_address]] detail::GlobalNodePtr<Context> globalNode;
        };

        template<class ImplNode>
        struct InnerContext : Context
        {
            static InnerContext isVirtualContext(detail::IsVirtualContextTag);

            template<IsNodeHandle T, class Current>
            requires (not std::is_const_v<Current>)
            static constexpr auto exchangeImpl(Current& current, auto&&... args)
            {
                return Impl<ImplNode>::getVirtualHost(std::addressof(current))->template exchangeImpl<T>(current, ARC_FWD(args)...);
            }

            template<class N, IsTrait Trait>
            requires detail::HasLink<Context, Trait>
            constexpr auto getNode(this Context virtualContext, N& impl, Trait trait)
            {
                Node* virtualHost = Impl<ImplNode>::getVirtualHost(std::addressof(impl));
                return virtualContext.getNode(std::forward_like<N&>(*virtualHost), trait);
            }

            template<IsGlobalTrait GlobalTrait>
            static constexpr auto getNode(auto& node, GlobalTrait)
            {
                detail::assertContextHasGlobalTrait<Context, GlobalTrait>();
                return getGlobalNode(node).asTrait(detail::AsRef{}, typename GlobalTrait::Trait{});
            }

            template<class N>
            static constexpr auto& getGlobalNode(N& impl)
            {
                // Global node pointer is cached in ImplBase to avoid multiple lookups
                return std::forward_like<N&>(*Impl<ImplNode>::getGlobalNode(std::addressof(impl)));
            }

            template<IsContext Parent>
            constexpr auto getParentMemPtr()
            {
                // Disable member pointer access to children as implementation can be swapped out at runtime
                static_assert(detail::alwaysFalse<Parent>, "arc::Virtual does not have a stable member pointer to its children");
            }

            template<IsContext Parent, class N>
            constexpr auto& getParentNode(this Context virtualContext, N& node)
            {
                if constexpr (std::is_same_v<detail::Decompress<Parent>, InnerContext>)
                {
                    return node;
                }
                else
                {
                    Node* virtualHost = Impl<ImplNode>::getVirtualHost(std::addressof(node));
                    return virtualContext.template getParentNode<Parent>(std::forward_like<N&>(*virtualHost));
                }
            }
        };

        template<IsNodeHandle T>
        using ImplOf = detail::ToVirtualNodeImpl<T, InnerContext<T>>;

        template<IsNodeHandle T>
        using ImplInterface = std::remove_pointer_t<decltype(std::declval<ImplOf<T>&>().asInterface())>;

        template<IsNodeHandle ImplNode>
        struct Impl final : ImplBase
        {
            explicit constexpr Impl(Node* virtualHost, auto&&... args)
                : ImplBase{virtualHost}
                , impl{ARC_FWD(args)...}
            {}

        private:
            friend Node;
            ImplOf<ImplNode> impl;

            static constexpr Node* getVirtualHost(ImplOf<ImplNode> const* p)
            {
                auto memPtr = ARC_MEM_PTR(Impl, impl);
                auto const& self = memPtr.getClassFromMember(*p);
                return self.ImplBase::getVirtualHost();
            }

            static constexpr auto* getGlobalNode(ImplOf<ImplNode> const* p)
            {
                auto memPtr = ARC_MEM_PTR(Impl, impl);
                auto const& self = memPtr.getClassFromMember(*p);
                return self.ImplBase::getGlobalNode();
            }

            void onGraphConstructed() final
            {
                impl.visit(detail::OnGraphConstructedVisitor{});
            }

            void visitExchangeImpl() final
            {
                auto visitor = []<class DN>(DN node)
                {
                    if constexpr (IsDynamicContext<ContextOf<DN>>)
                        node.exchangeImpl();
                };
                impl.visit(detail::TraitVisitor<trait::DynamicNode, decltype(visitor)>{visitor});
            }

            void setVirtualHost(Node* newVirtualHost) final
            {
                ImplBase::setVirtualHost(newVirtualHost);
                if constexpr (detail::injectVirtualHost<ImplNode>)
                    impl.setVirtualHost(newVirtualHost);
            }
        };

        template<class Trait>
        using InterfaceOf = detail::SelectIf<
            detail::NodeTraitsHasTraitPred<Trait>,
            Interfaces...
        >;

        template<class Interface>
        struct AsInterface;

        template<class Trait>
        struct Resolver;

        template<class Trait>
        requires (... || detail::TraitsHasTrait<typename Interfaces::Traits, Trait>)
        struct Resolver<Trait>
        {
            using TraitInterface = InterfaceOf<Trait>;
            using Types = TraitInterface::Traits::template ResolveTypes<Trait>;
            using Interface = AsInterface<TraitInterface>;
            ARC_ASSERT_IMPLEMENTS(TraitInterface, Types, Trait);
        };

        // arc::Virtual implements arc::trait::DynamicNode::exchangeImpl by visiting all contained nodes
        // that also implement arc::trait::DynamicNode and calling their exchangeImpl method.
        // This allows nested arc::Virtual graphs to exchange their implementation in a single pass.
        template<std::same_as<trait::DynamicNode> Trait>
        struct Resolver<Trait>
        {
            using Types = EmptyTypes;
            using Interface = Node;
        };

    public:
        using Traits = arc::TraitsTemplate<Node, Resolver>;

        template<IsNodeHandle T>
        requires (... and std::derived_from<ImplInterface<T>, Interfaces>)
        explicit(false) constexpr Node(std::in_place_type_t<T>, auto&&... args)
        {
            if constexpr (detail::injectVirtualHost<T>)
                [[maybe_unused]] auto ignored = init<T>(this, ARC_FWD(args)...);
            else
                [[maybe_unused]] auto ignored = init<T>(ARC_FWD(args)...);
        }

        template<std::invocable<Constructor<Node>> F>
        requires std::same_as<Node, std::invoke_result_t<F, Constructor<Node>>>
        explicit constexpr Node(WithFactory, F factory)
            : Node(factory(Constructor<Node>()))
        {}

        template<IsNodeHandle T>
        requires (... and std::derived_from<ImplInterface<T>, Interfaces>)
        constexpr ImplOf<T>& emplace(auto&&... args)
        {
            if constexpr (ContextHasGlobalTrait<Context, Global<trait::Scheduler>>)
            {
                if (not getGlobal(trait::scheduler).inExclusiveMode())
                    throw std::runtime_error("arc::Union::emplace can only be called when the scheduler is in exclusive mode");
            }

            if constexpr (detail::injectVirtualHost<T>)
            {
                auto [next, prev] = init<T>(this, ARC_FWD(args)...);
                onGraphConstructed();
                return next->impl;
            }
            else
            {
                auto [next, prev] = init<T>(ARC_FWD(args)...);
                onGraphConstructed();
                return next->impl;
            }
        }

        Node(Node const&) = delete;
        constexpr Node(Node&& other)
            : interfaces(other.interfaces)
            , implBase(std::move(other.implBase))
        {
            other.interfaces = {};
            if (implBase)
                implBase->setVirtualHost(this);
        }

        constexpr void onGraphConstructed()
        {
            implBase->onGraphConstructed();
        }

        constexpr void impl(trait::DynamicNode::exchangeImpl)
        {
            implBase->visitExchangeImpl();
        }

    protected:
        std::tuple<Interfaces*...> interfaces;
        std::unique_ptr<ImplBase> implBase;

        template<IsNodeHandle T>
        requires (... and std::derived_from<ImplInterface<T>, Interfaces>)
        [[nodiscard]] constexpr auto init(auto&&... args)
        {
            Impl<T>* p = new Impl<T>(this, ARC_FWD(args)...);
            // Updates pointers if new didn't throw
            auto* pimpl = p->impl.asInterface();
            interfaces = {static_cast<Interfaces*>(pimpl)...};
            return std::pair(p, std::exchange(implBase, std::unique_ptr<ImplBase>(p)));
        }

        // RAII placeholder when exchange is deferred via scheduler
        struct ExchangedDeferred
        {
            constexpr KeepAlive keepAlive() { return {}; }
            constexpr operator KeepAlive() { return keepAlive(); }
        };

        template<IsNodeHandle T, class Current>
        constexpr auto exchangeImpl(Current& current, auto&&... args)
        {
            if (get<0>(interfaces) != current.asInterface()) [[unlikely]]
                throw std::runtime_error("Not exchanging the current interface");

            if constexpr (ContextHasGlobalTrait<Context, Global<trait::Scheduler>>)
            {
                static_assert(ContextHasTrait<ContextOf<Current>, trait::DynamicNode>,
                    "Node inside arc::Virtual can only exchange its implementation with a scheduler when it has the arc::trait::DynamicNode trait");
                auto scheduler = this->getGlobal(trait::scheduler);
                if (not scheduler.inExclusiveMode())
                {
                    // Defer actual exchange until exclusive mode; schedule a pass over dynamic nodes.
                    scheduler.postExclusiveTask(
                        trait::DynamicNode::exchangeImpl{},
                        Context{}.template getParentNode<typename Context::Root::Context>(*this),
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
                    // The returned Exchanged object is deferred; callers must check deferred()/empty() before using getNext().
                    return Exchanged(nullptr, nullptr);
                }
            }

            // Immediate (exclusive or no scheduler) path
            auto [next, prev] = init<T>(ARC_FWD(args)...);
            onGraphConstructed();
            return Exchanged(std::addressof(next->impl), std::move(prev));
        }

        template<class Next>
        struct [[nodiscard, maybe_unused]] Exchanged
        {
            constexpr Exchanged(Next* next, std::unique_ptr<ImplBase> previous)
                : next(next)
                , previous(std::move(previous))
            {}

            // Do not allow reference to next to escape block scope
            // to avoid implicitly holding it somewhere as a dangling reference
            Exchanged(Exchanged&&) = delete;

            // Allow extension of previous lifetime beyond the current block
            constexpr KeepAlive keepAlive() { return KeepAlive(std::move(previous)); }
            constexpr operator KeepAlive() { return keepAlive(); }

            // Access to next only allowed when ExchangeHandle is an lvalue to ensure that
            // previous lifetime has at least been extended to the block scope of the exchangeImpl caller
            constexpr Next* getNext() const & { return next; }

            constexpr bool empty() const { return next == nullptr; }
            constexpr bool deferred() const { return empty(); }
            constexpr operator bool() const { return not empty(); }

        private:
            Next* next;
            std::unique_ptr<ImplBase> previous;
        };
    };
};

template<>
struct Virtual<>
{
    template<class Context>
    struct Node : arc::Node
    {
        using Traits = arc::Traits<Node>;
        constexpr explicit Node(auto&&...) {}
        template<class>
        constexpr void emplace(auto&&...) {}
    };
};

template<IsInterface... Interfaces>
template<class Context>
template<class Interface>
struct Virtual<Interfaces...>::Node<Context>::AsInterface : Node
{
    template<class Self>
    ARC_INLINE constexpr decltype(auto) impl(this Self& self, auto&&... args)
    {
        return std::forward_like<Self&>(*get<Interface*>(self.interfaces)).impl(ARC_FWD(args)...);
    }
};

} // namespace arc

#endif // INCLUDE_ARC_VIRTUAL_HPP
