#ifndef INCLUDE_ARC_NARROW_HPP
#define INCLUDE_ARC_NARROW_HPP

#include "arc/cluster.hpp"
#include "arc/context.hpp"
#include "arc/ensure.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"
#include "arc/trait.hpp"

namespace arc {

ARC_MODULE_EXPORT
template<IsNodeHandle NodeHandle, IsTrait... Traits>
struct Narrow
{
    template<class Context>
    struct Cluster : arc::Cluster
    {
        static constexpr bool isUnary() { return decltype(node)::isUnary(); }

        struct Inner : arc::Context<Cluster, NodeHandle>
        {
            static constexpr std::size_t Depth = Context::Depth;

            template<class T>
            requires detail::HasLocalLink<Context, T>
            static auto resolveLink(T, LinkPriorityMin) -> ResolvedLink<Context, T>;
        };
        ARC_NODE(Inner, node);

        // Narrow exposed traits
        template<class T>
        requires (... || MatchesTrait<T, Traits>)
        static auto resolveLink(T, LinkPriorityMin) -> ResolvedLink<Inner, T>;

        constexpr decltype(auto) operator->(this auto& self)
        {
            if constexpr (IsNode<ContextToNode<Inner>>)
                return (self.node);
            else
                return std::addressof(self.node);
        }

        constexpr void visit(this auto& self, auto&& visitor)
        {
            self.node.visit(ARC_FWD(visitor));
        }
    };

    template<class Context>
    using Node = Cluster<Context>;
};

namespace node {
    ARC_MODULE_EXPORT
    using arc::Narrow;
}

}


#endif // INCLUDE_ARC_NARROW_HPP
