#ifndef INCLUDE_ARC_ENSURE_HPP
#define INCLUDE_ARC_ENSURE_HPP

#include "arc/cluster_fwd.hpp"
#include "arc/count.hpp"
#include "arc/empty_types.hpp"
#include "arc/global_context.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"
#include "arc/static_assert.hpp"
#include "arc/trait.hpp"

namespace arc {

namespace detail {
    template<class T>
    consteval void assertNodeDependenciesSatisfied()
    {
        if constexpr (NodeDependenciesSatisfied<T>)
            return;
        // A missing global trait, in a cluster without its link to the global node, is that link missing
        else if constexpr (not ContextHasGlobalConnection<ContextOf<T>>
                           and missingDependencies<T, false>().find("arc::Global<") != std::string::npos)
            ARC_STATIC_ASSERT_INLINE(NodeDependenciesSatisfied<T>)(
                "Node depends on a global trait, but its cluster does not link it to the global node: "
                "add `[[@global]] @all ==> @global` to the cluster or add a link for this specific trait",
                ARC_SA_NODE_CONTEXT(node, T)(missing, missingDependencies<T, false>()));
        else
            ARC_STATIC_ASSERT_INLINE(NodeDependenciesSatisfied<T>)(
                "Not all entries in node's arc::Depends<>/Uses<> are satisfied by its cluster",
                ARC_SA_NODE_CONTEXT(node, T)(missing, missingDependencies<T, false>()));
    }
}

ARC_MODULE_EXPORT
template<class T, class... Pred>
requires ((detail::assertNodeDependenciesSatisfied<T>(), ..., Pred::template assert<T>()), true)
using Ensure = T;

namespace pred {

    ARC_MODULE_EXPORT
    struct Unary
    {
        template<class Node>
        static consteval void assert()
        {
            ARC_STATIC_ASSERT_INLINE(IsUnary<Node>)("Domain node name is not all upper-case, so the node must be unary",
                ARC_SA_NODE_CONTEXT(node, Node));
        }
    };

    ARC_MODULE_EXPORT
    struct NonUnary
    {
        template<class Node>
        static consteval void assert()
        {
            ARC_STATIC_ASSERT_INLINE(not IsUnary<Node>)("Domain node name is all upper-case, so the node must not be unary",
                ARC_SA_NODE_CONTEXT(node, Node));
        }
    };

    ARC_MODULE_EXPORT
    struct Stateless
    {
        template<class Node>
        static consteval void assert()
        {
            ARC_STATIC_ASSERT_INLINE(IsStateless<Node>)("Domain node name starts with a lower-case letter, so the node must be stateless",
                ARC_SA_NODE_CONTEXT(node, Node)(size, sizeof(Node)));
        }
    };

    ARC_MODULE_EXPORT
    struct HasDepends
    {
        template<class Node>
        static consteval void assert()
        {
            ARC_STATIC_ASSERT_INLINE(NodeHasDepends<Node>)("Domain node must list its dependencies with arc::Depends<> or arc::Node::Uses<>",
                ARC_SA_NODE_CONTEXT(node, Node));
        }
    };

    ARC_MODULE_EXPORT
    template<IsTrait... Traits>
    struct HasTraits
    {
        template<class Node>
        static consteval void assert()
        {
            (assertTrait<Node>(Traits()), ...);
        }
        template<class Node, class... TrunkTrait>
        static consteval void assertTrait(Trunk<TrunkTrait...>)
        {
            (assertTrait<Node>(TrunkTrait()), ...);
        }
        template<class Node, class Trait>
        static consteval void assertTrait(Trait)
        {
            if constexpr (IsCluster<Node>)
            {
                ARC_STATIC_ASSERT_INLINE(HasTrait<Node, Trait>)(
                    "Cluster provides no member that supplies a trait its parent requires; check its links",
                    (trait, ::arc::typeName<Trait>) \
                    (cluster, ::arc::detail::staticAssertClusterLocation<Node>()) \
                    (parent, ::arc::detail::staticAssertParentCluster<Node>()));
            }
            else
            {
                ARC_STATIC_ASSERT_INLINE(HasTrait<Node, Trait>)(
                    "Node does not list a client's Trait in arc::Traits<> or arc::Node::Impl<>",
                    (trait, ::arc::typeName<Trait>) \
                    ARC_SA_NODE_CONTEXT(node, Node));
            }
        }
    };
}

}

#endif // INCLUDE_ARC_ENSURE_HPP
