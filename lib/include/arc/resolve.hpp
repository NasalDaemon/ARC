#ifndef INCLUDE_ARC_RESOLVE_HPP
#define INCLUDE_ARC_RESOLVE_HPP

#include "arc/global_context.hpp"
#include "arc/global_trait.hpp"
#include "arc/link.hpp"
#include "arc/context_fwd.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"

namespace arc {

namespace detail {

    // T has no link, so it is the target node
    template<class T, class Trait_>
    struct ResolveTraitT
    {
        using type = ResolveTraitT;
        using Node = T;
        using Trait = Trait_;

        using Types = Node::Traits::template ResolveTypes<Trait>;
    };

    // GlobalTrait is a global trait that can be resolved in contexts with a global node
    template<class T, IsGlobalTrait GlobalTrait>
    struct ResolveTraitT<T, GlobalTrait>
    {
        static_assert(ContextHasGlobalTrait<T, GlobalTrait>,
                      "Global trait can only be resolved in graphs that have a global node");
        using Trait = GlobalTrait::Trait;
        using type = ResolveTraitT<typename T::Info::GlobalNode, Trait>::type;
    };

    // T is a context with a link to a sibling node's context
    template<class T, class Trait>
    requires HasLink<T, Trait>
    struct ResolveTraitT<T, Trait>
    {
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

    template<class T, class Trait>
    using ResolveTrait = ResolveTraitT<T, Trait>::type;

} // namespace detail

ARC_MODULE_EXPORT
template<class Node, IsTrait Trait>
requires detail::HasLink<ContextOf<Node>, Trait> and NodeDependencyAllowed<Node, Trait>
using ResolveTypes = detail::ResolveTrait<ContextOf<Node>, Trait>::Types;

ARC_MODULE_EXPORT
template<IsContext Context>
using ResolveRoot = Context::Root;

ARC_MODULE_EXPORT
template<IsContext Context>
using ResolveInfo = Context::Info;

} // namespace arc


#endif // INCLUDE_ARC_RESOLVE_HPP
