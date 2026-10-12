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

    // T is a context with a link to a sibling node's context
    template<class T, class Trait>
    requires LocallyResolvable<T, Trait>
    struct ResolveTraitT<T, Trait>
    {
        ARC_STATIC_ASSERT(not IsGlobalTrait<Trait> or test::IsTestContext<T>)("Global traits may only be resolved locally in a test context",
            ARC_SA_NODE_CONTEXT(node, T)(trait, arc::typeName<Trait>));
        using Target = ResolveLink<T, Trait>;
        using type = ResolveTraitT<ContextToNode<typename Target::Context>, typename Target::Trait>::type;
    };

    // T links to parent context
    template<class T, class Trait>
    requires LinksToParent<T, Trait>
    struct ResolveTraitT<T, Trait>
    {
        using Target = ResolveLink<T, Trait>;
        using type = ResolveTraitT<typename Target::Context, typename Target::Trait>::type;
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
