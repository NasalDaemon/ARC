#ifndef INCLUDE_ARC_RESOLVE_HPP
#define INCLUDE_ARC_RESOLVE_HPP

#include "arc/global_context.hpp"
#include "arc/global_trait.hpp"
#include "arc/link.hpp"
#include "arc/context_fwd.hpp"
#include "arc/macros.hpp"
#include "arc/static_assert.hpp"
#include "arc/node_fwd.hpp"
#include "arc/test_context.hpp"

namespace arc {

namespace detail {

    // normal sibling link or a global trait resolving to a local mock in a test context
    template<class T, class Trait>
    concept LocallyResolvable = HasLocalLink<T, Trait>
                            or (IsGlobalTrait<Trait> and not ContextHasGlobal<T> and test::IsTestContext<T> and HasExplicitLink<T, Trait>);

    // T has no link, so it is the target node
    template<class Node_, class Trait_>
    struct ResolveTraitT
    {
        using type = ResolveTraitT;

        using Node = Node_;
        using Trait = Trait_;

        template<std::same_as<Trait> TT = Trait>
        static constexpr bool HasTrait = Node::Traits::template HasTrait<TT>;

        template<class Source, class... Keys>
        using Types = Node::template FinaliseTypes<Source, Node, typename Node::Traits::template ResolveTypes<Trait, Node>, Keys...>;
    };

    // GlobalTrait is a global trait that can be resolved in contexts with a global node
    template<class T, IsGlobalTrait GlobalTrait>
    requires ContextHasGlobal<T>
    struct ResolveTraitT<T, GlobalTrait>
    {
        using Trait = GlobalTrait::Trait;
        using type = ResolveTraitT<typename T::Info::GlobalNode, Trait>::type;
    };

    // The context and trait a link from T arrives at, past the clusters passing it up. Next is where resolution goes on
    // from: the node of that context, or the context itself when a cluster passes the trait up to its global node.
    // Node types are only named here, never completed.
    template<class T, class Trait_>
    struct LinkTarget : ResolveLink<T, Trait_>
    {
        using Next = ContextToNode<typename ResolveLink<T, Trait_>::Context>;
    };

    template<class T, IsGlobalTrait Trait_>
    requires ContextHasGlobal<T>
    struct LinkTarget<T, Trait_>
    {
        using Context = T;
        using Trait = Trait_;
        using Next = T;
    };

    template<class T, class Trait_>
    requires LinksToParent<T, Trait_>
    struct LinkTarget<T, Trait_> : LinkTarget<ResolveLinkContext<T, Trait_>, ResolveLinkTrait<T, Trait_>>
    {};

    enum class LinkReach { No, Yes, ThroughElement };

    // Whether Trait's calls arriving at context Target reach the node of context C: C is Target, or C's wrapper or
    // cluster is reached and hosts C or passes Trait on to it. A collection's elements are never reached:
    // ThroughElement when they would otherwise be.
    template<class C, class Target, class Trait>
    consteval LinkReach linkReach()
    {
        using Ctx = Decompress<C>;
        if constexpr (IsSameContext<Ctx, Target>)
        {
            return LinkReach::Yes;
        }
        else if constexpr (requires { typename Ctx::NodePath::Context; })
        {
            constexpr LinkReach hostReach = linkReach<typename Ctx::NodePath::Context, Target, Trait>();
            return Ctx::NodePath::IsElement and hostReach != LinkReach::No ? LinkReach::ThroughElement : hostReach;
        }
        else if constexpr (requires { typename Ctx::Parent; } and HasExplicitLink<typename Ctx::Parent, Trait>)
        {
            if constexpr (IsSameContext<ResolveLinkContext<typename Ctx::Parent, Trait>, Ctx>)
                return linkReach<ContextOf<typename Ctx::Parent>, Target, Trait>();
            else
                return LinkReach::No;
        }
        else
        {
            return LinkReach::No;
        }
    }

    // T is a context with a link to a node's context
    template<class T, class Trait>
    requires LocallyResolvable<T, Trait>
    struct ResolveTraitT<T, Trait>
    {
        ARC_STATIC_ASSERT(not IsGlobalTrait<Trait> or test::IsTestContext<T>)("Global traits may only be resolved locally in a test context",
            ARC_SA_NODE_CONTEXT(node, T)(trait, arc::typeName<Trait>));
        using Target = LinkTarget<T, Trait>;
        using type = ResolveTraitT<typename Target::Next, typename Target::Trait>::type;
    };

    template<class Node, class Trait, class... Keys>
    using ResolveTypesOfNode = ResolveTraitT<Node, Trait>::type::template Types<Node, Keys...>;

    template<class Node, class Trait>
    using ResolveTraitFromNode = ResolveTraitT<ContextOf<Node>, Trait>::type;

    // For arc::ResolveTypes: a hard error naming the fix, rather than an unsatisfied constraint
    template<class Node, class Trait>
    consteval void assertCanResolveTypes()
    {
        // Link first: checking the listing needs the node complete, which it is not while its class body resolves types
        if constexpr (IsGlobalTrait<Trait>)
        {
            ARC_STATIC_ASSERT_INLINE(HasLink<ContextOf<Node>, Trait>)(
                "Node depends on a global trait, but its cluster does not link it to the global node: "
                "add `[[@global]] @all ==> @global` to the cluster or add the specific connection to global",
                ARC_SA_NODE_CONTEXT(node, ContextOf<Node>)(trait, arc::typeName<Trait>));
        }
        else
        {
            ARC_STATIC_ASSERT_INLINE(HasLink<ContextOf<Node>, Trait>)(
                "Node resolves the types of a trait that its cluster does not connect it to",
                ARC_SA_NODE_CONTEXT(node, ContextOf<Node>)(trait, arc::typeName<Trait>));
        }
        if constexpr (HasLink<ContextOf<Node>, Trait>)
        {
            ARC_STATIC_ASSERT_INLINE(NodeDependencyListed<Node, Trait>)(
                "Node resolves the types of a trait that is not listed in its arc::Depends<>/Uses<>",
                ARC_SA_NODE_CONTEXT(node, ContextOf<Node>)(trait, arc::typeName<Trait>));
        }
    }

} // namespace detail

ARC_MODULE_EXPORT
template<class Node, class Trait>
concept CanResolve =
    detail::HasLink<ContextOf<Node>, Trait> and
    NodeDependencyListed<Node, Trait> and
    detail::ResolveTraitFromNode<Node, Trait>::template HasTrait<>;

ARC_MODULE_EXPORT
template<class Node, IsTrait Trait, class... Keys>
requires (detail::assertCanResolveTypes<Node, Trait>(), true)
using ResolveTypes = detail::ResolveTraitFromNode<Node, Trait>::template Types<Node, Keys...>;

ARC_MODULE_EXPORT
template<IsContext Context>
using ResolveRoot = Context::Root;

ARC_MODULE_EXPORT
template<IsContext Context>
using ResolveInfo = Context::Info;

} // namespace arc


#endif // INCLUDE_ARC_RESOLVE_HPP
