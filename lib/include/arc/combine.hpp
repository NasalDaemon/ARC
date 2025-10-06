#ifndef INCLUDE_ARC_COMBINE_HPP
#define INCLUDE_ARC_COMBINE_HPP

#include "arc/detail/cast.hpp"
#include "arc/detail/select.hpp"
#include "arc/detail/type_at.hpp"
#include "arc/cluster.hpp"
#include "arc/context.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/trait.hpp"

#if !ARC_IMPORT_STD
#include <cstdint>
#include <type_traits>
#endif

namespace arc {

namespace detail {

    template<class CombineNode, IsNodeHandle Node>
    struct CombinePart
    {
        struct Context : arc::Context<CombineNode, Node>
        {
            using ParentContext = ContextParameterOf<CombineNode>;

            template<class T>
            requires detail::HasLink<ParentContext, T>
            static auto resolveLink(T) -> ResolvedLink<ParentContext, T>;
        };

        [[no_unique_address]] ContextToNodeState<Context> node{};

        friend ARC_IF_MSVC_ELSE(constexpr)(consteval) auto getNodePointer(arc::AdlTag<Context>)
        {
            return memberPtr<CombineNode>(&CombinePart::node);
        }
    };

} // namespace detail

ARC_MODULE_EXPORT
template<IsNodeHandle... Nodes>
struct Combine
{
    template<class Context>
    struct Node : Cluster, detail::CombinePart<Node<Context>, Nodes>...
    {
        static constexpr bool isUnary()
        {
            return (sizeof...(Nodes) == 1) and (... and (decltype(detail::CombinePart<Node<Context>, Nodes>::node)::isUnary()));
        }

        template<IsTrait Trait>
        static auto resolveLink(Trait)
            -> ResolvedLink<
                detail::SelectIf<
                    ContextHasTraitPred<Trait>,
                    typename detail::CombinePart<Node, Nodes>::Context...
                >,
                Trait>;

        template<IsNodeHandle N>
        requires (... || std::is_same_v<N, Nodes>)
        auto& get(this auto& self)
        {
            return detail::upCast<detail::CombinePart<Node, N>>(self).node;
        }

        template<std::size_t I>
        requires (I < sizeof...(Nodes))
        auto& get(this auto& self)
        {
            return detail::upCast<detail::CombinePart<Node, detail::TypeAt<I, Nodes...>>>(self).node;
        }

        constexpr void visit(this auto& self, auto&& visitor)
        {
            (self.template get<Nodes>().visit(visitor), ...);
        }
    };
};

} // namespace arc


#endif // INCLUDE_ARC_COMBINE_HPP
