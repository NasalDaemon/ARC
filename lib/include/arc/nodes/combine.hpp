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
            requires detail::HasLocalLink<ParentContext, T>
            static auto resolveLink(T, LinkPriorityMin) -> ResolvedLink<ParentContext, T>;
        };

        [[no_unique_address]] ContextToNodeState<Context> node{};

        friend ARC_IF_MSVC_ELSE(constexpr)(consteval) auto getNodePointer(arc::AdlTag<Context>)
        {
            return memberPtr<CombineNode>(&CombinePart::node);
        }
    };

} // namespace detail

ARC_MODULE_EXPORT
template<IsNodeHandle Fallback, IsNodeHandle... Nodes>
struct Combine
{
    template<class Context>
    struct Cluster : arc::Cluster, detail::CombinePart<Cluster<Context>, Nodes>..., detail::CombinePart<Cluster<Context>, Fallback>
    {
        static constexpr bool isUnary()
        {
            return (sizeof...(Nodes) == 0) and decltype(detail::CombinePart<Cluster, Fallback>::node)::isUnary();
        }

        template<IsTrait Trait>
        requires (... || ContextHasTrait<typename detail::CombinePart<Cluster, Nodes>::Context, Trait>)
        static auto resolveLink(Trait, LinkPriorityMax)
            -> ResolvedLink<
                detail::SelectIf<
                    ContextHasTraitPred,
                    Trait,
                    typename detail::CombinePart<Cluster, Nodes>::Context...
                >,
                Trait>;

        template<IsTrait Trait>
        requires ContextHasTrait<typename detail::CombinePart<Cluster, Fallback>::Context, Trait>
        static auto resolveLink(Trait, LinkPriorityMin)
            -> ResolvedLink<typename detail::CombinePart<Cluster, Fallback>::Context, Trait>;

        template<IsNodeHandle N>
        requires (std::is_same_v<N, Fallback> || ... || std::is_same_v<N, Nodes>)
        auto& get(this auto& self)
        {
            return detail::upCast<detail::CombinePart<Cluster, N>>(self).node;
        }

        template<std::size_t I>
        requires (I <= sizeof...(Nodes))
        auto& get(this auto& self)
        {
            return detail::upCast<detail::CombinePart<Cluster, detail::TypeAt<I, Fallback, Nodes...>>>(self).node;
        }

        constexpr void visit(this auto& self, auto&& visitor)
        {
            self.template get<Fallback>().visit(visitor);
            (self.template get<Nodes>().visit(visitor), ...);
        }
    };

    template<class Context>
    using Node = Cluster<Context>;
};

namespace node {
    ARC_MODULE_EXPORT
    using arc::Combine;
}

} // namespace arc


#endif // INCLUDE_ARC_COMBINE_HPP
