#ifndef INCLUDE_ARC_NODE_HPP
#define INCLUDE_ARC_NODE_HPP

#include "arc/detail/as_ref.hpp"
#include "arc/detail/cast.hpp"

#include "arc/context_fwd.hpp"
#include "arc/depends.hpp"
#include "arc/empty_types.hpp"
#include "arc/environment.hpp"
#include "arc/factory.hpp"
#include "arc/global_trait.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"
#include "arc/trait.hpp"
#include "arc/trait_view.hpp"
#include "arc/traits_fwd.hpp"

#if !ARC_IMPORT_STD
#include <functional>
#include <type_traits>
#endif

namespace arc {

struct Node
{
    static constexpr bool isUnary() { return true; }
    using Types = EmptyTypes;
    using Environment = arc::Environment<>;
    using Depends = detail::DependsImplicitly;

    // Also exposed in TraitNodeView
    template<class Self, std::invocable<Self&> Visitor>
    ARC_INLINE constexpr decltype(auto) visit(this Self& self, Visitor&& visitor)
    {
        return std::invoke(ARC_FWD(visitor), self);
    }

    ARC_INLINE constexpr auto& getState(this auto& self) { return self; }

#if ARC_AUTOCOMPLETE
    template<IsTrait Trait, class Key = key::Default>
    static constexpr AutoCompleteTraitView<key::Trait<Key, Trait>> getNode(Trait trait = {}, Key const& key = {}, auto const&... keys);
#else
    template<IsTrait Trait, class Self, class Key = ContextOf<Self>::Info::DefaultKey>
    constexpr IsTraitViewOf<Trait, Key> auto getNode(this Self& self, Trait trait = {}, Key const& key = {}, auto const&... keys)
    {
        Self::template assertNodeContext<Self>();
        if constexpr (not detail::IsNodeState<Self>)
            static_assert(NodeDependencyListed<Self, Trait>, "Requested trait missing from node's arc::Depends list");
        using ThisNode = Self::Traits::Node;
        auto& node = detail::upCast<ThisNode>(self);
        if constexpr (not IsGlobalTrait<Trait>)
            static_assert(detail::HasLink<ContextOf<Self>, Trait>, "Node missing link to a dependency");
        auto target = ContextOf<Self>{}.getNode(node, trait);
        return makeTraitView(self, target, trait, key, keys...);
    }
#endif

    template<IsTrait Trait, class Self, class Key = ContextOf<Self>::Info::DefaultKey>
    constexpr auto getGlobal(this Self& self, Trait trait = {}, Key const& key = {}, auto const&... keys)
    {
        return self.getNode(arc::global(trait), key, keys...);
    }

    template<IsTrait Trait, class Self>
    constexpr auto canGetNode(this Self&, Trait = {})
    {
        constexpr bool value =
            requires (ContextOf<Self> c, Self::Traits::Node n, Trait trait) {
                c.getNode(n, trait);
            };
        return std::bool_constant<value>{};
    }

    template<IsTrait Trait, class Self, class Key = ContextOf<Self>::Info::DefaultKey>
    constexpr IsTraitViewOf<Trait, Key> auto asTrait(this Self& self, Trait trait = {}, Key const& key = {}, auto const&... keys)
    {
        auto impl = self.asTrait(detail::AsRef{}, trait);
        return makeTraitView(self, impl, trait, key, keys...);
    }

    template<class Self, IsTrait Trait>
    constexpr auto asTrait(this Self& self, detail::AsRef, Trait)
    {
        Self::template assertNodeContext<Self>();
        using ThisNode = Self::Traits::Node;
        auto& node = detail::upCast<ThisNode>(self);

        static_assert(Self::Traits::template HasTrait<Trait>, "Trait not listed in node's arc::Traits");
        using Interface = Self::Traits::template ResolveInterface<Trait>;
        using Types = Self::Traits::template ResolveTypes<Trait>;
        return detail::TargetRef{detail::downCast<Interface>(node), std::type_identity<Types>{}};
    }

    template<IsTrait Trait, class Self>
    constexpr auto hasTrait(this Self&, Trait = {}) -> std::bool_constant<detail::TraitsHasTrait<typename Self::Traits, Trait>>
    {
        return {};
    }

    template<class Self, class Key = ContextOf<Self>::Info::DefaultKey>
    constexpr auto finalise(this Self& self, auto& source, Key const& key = {}, auto const&... keys)
    {
        return ContextOf<Self>::Info::finalise(source, self, key, keys...);
    }

    template<class Self>
    static consteval void assertNodeContext()
    {
        static_assert(NodeDependenciesSatisfied<Self, true>, "Not all entries in node's arc::Depends are satisfied transitively.");
    }

    // For use by nodes host in arc::Union or arc::Virtual
    template<IsNodeHandle T, class Self>
    constexpr decltype(auto) exchangeImpl(this Self& self, auto&&... args)
    {
        static_assert(not std::is_const_v<Self>);
        return ContextOf<Self>::template exchangeImpl<T>(self, ARC_FWD(args)...);
    }

    // arc::Peer will override these
    static void getElementId() = delete;
    static void getElementHandle() = delete;
    static void getPeers() = delete;
};

template<IsNode NodeT>
struct WrapNode
{
    template<class Context>
    using Node = WrappedImpl<NodeT, Context>;

    template<class Context>
    struct GetTraits
    {
        template<std::same_as<NodeT> T>
        using GetContext = Context;

        template<class Trait>
        requires detail::TraitsHasTrait<typename NodeT::Traits, Trait>
        struct Resolver
        {
            using Types = NodeT::Traits::template ResolveTypes<Trait>;
            using Interface = WrappedImpl<typename NodeT::Traits::template ResolveInterface<Trait>, Context>;
        };

        using Traits = detail::Traits<NodeT, GetContext, detail::TraitsTemplateDefault<Resolver>>;
    };

    template<class Context>
    using Traits = GetTraits<Context>::Traits;
};

namespace detail {

    template<IsNode Node>
    struct NodeState : private Node
    {
        using Node::Node;

        template<class F>
        explicit constexpr NodeState(Emplace<F> const& f)
            : Node(f)
        {}

        ARC_NODE_USE_PUBLIC_MEMBERS(Node)

        template<class Self>
        ARC_INLINE constexpr decltype(auto) visit(this Self& self, auto&& f)
        {
            ContextOf<Self>::Info::assertVisitable(self);
            return upCast<Node>(self).visit(ARC_FWD(f));
        }

        template<class Self>
        ARC_INLINE constexpr auto& getState(this Self& self)
        {
            ContextOf<Self>::Info::assertAccessible(self);
            return upCast<Node>(self).getState();
        }

        constexpr auto* operator->(this auto& self) { return std::addressof(self.getState()); }
    };

} // namespace detail

} // namespace arc


#endif // INCLUDE_ARC_NODE_HPP
