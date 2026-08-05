#ifndef INCLUDE_ARC_RESOLVE_HPP
#define INCLUDE_ARC_RESOLVE_HPP

#include "arc/global_context.hpp"
#include "arc/global_trait.hpp"
#include "arc/link.hpp"
#include "arc/context_fwd.hpp"
#include "arc/macros.hpp"
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
        static_assert(not IsGlobalTrait<Trait> or test::IsTestContext<T>, "Global traits may only be resolved locally in a test context");
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

} // namespace detail

ARC_MODULE_EXPORT
template<class Node, class Trait>
concept CanResolve =
    detail::HasLink<ContextOf<Node>, Trait> and
    NodeDependencyListed<Node, Trait> and
    detail::ResolveTraitFromNode<Node, Trait>::template HasTrait<>;

ARC_MODULE_EXPORT
template<class Node, IsTrait Trait, class... Keys>
requires detail::HasLink<ContextOf<Node>, Trait> and NodeDependencyListed<Node, Trait>
using ResolveTypes = detail::ResolveTraitFromNode<Node, Trait>::template Types<Node, Keys...>;

ARC_MODULE_EXPORT
template<IsContext Context>
using ResolveRoot = Context::Root;

ARC_MODULE_EXPORT
template<IsContext Context>
using ResolveInfo = Context::Info;

} // namespace arc


#endif // INCLUDE_ARC_RESOLVE_HPP
