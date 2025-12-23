#ifndef INCLUDE_ARC_NARROW_HPP
#define INCLUDE_ARC_NARROW_HPP

#include "arc/cluster.hpp"
#include "arc/context.hpp"
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
    struct Node : Cluster
    {
        static constexpr bool isUnary() { return decltype(node)::isUnary(); }

        struct Inner : arc::Context<Node, NodeHandle>
        {
            static constexpr std::size_t Depth = Context::Depth;

            template<class T>
            requires detail::HasLink<Context, T>
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
};

}


#endif // INCLUDE_ARC_NARROW_HPP
