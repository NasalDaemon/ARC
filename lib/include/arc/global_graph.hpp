#ifndef INCLUDE_ARC_GLOBAL_HPP
#define INCLUDE_ARC_GLOBAL_HPP

#include "arc/detail/as_ref.hpp"

#include "arc/context.hpp"
#include "arc/cluster.hpp"
#include "arc/graph.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#include <memory>
#endif

namespace arc {

namespace detail {
    template<IsNodeHandle MainCluster, IsNodeHandle GlobalNodeHandle>
    struct GraphWithGlobal
    {
        template<class RootContext>
        class Cluster : public arc::Cluster
        {
            struct GlobalContext;
            struct MainContext;

            struct GlobalContextInfo;
            struct MainContextInfo;

            struct GlobalContext : arc::Context<Cluster, GlobalNodeHandle>
            {
                using Info = GlobalContextInfo;

                template<class Trait>
                requires (not IsGlobalTrait<Trait>)
                static ResolvedLink<MainContext, Trait> resolveLink(Trait, arc::LinkPriorityMin);

                template<class N>
                static constexpr auto& getGlobalNode(N& node)
                {
                    return downCast<decltype(Cluster::global)>(node);
                }
            };

            struct MainContext : arc::Context<Cluster, MainCluster>
            {
                using Info = MainContextInfo;

                template<class N>
                static constexpr auto& getGlobalNode(N& node)
                {
                    auto memPtr = getNodePointer(AdlTag<MainContext>{});
                    return memPtr.getClassFromMember(node).global;
                }
            };

        public:
            template<class T>
            static auto resolveLink(T, LinkPriorityMin) -> ResolvedLink<MainContext, T>;

            ARC_NODE(GlobalContext, global)
            ARC_NODE(MainContext, main)

            static_assert(IsCluster<decltype(main)>, "MainCluster must be a cluster type");

            constexpr auto* operator->(this auto& self)
            {
                return std::addressof(self.main);
            }

            template<class Self>
            constexpr void visit(this Self& self, auto&& visitor)
            {
                self.global.visit(visitor);
                self.main.visit(visitor);
            }

        private:
            struct MainContextInfo : RootContext::Info
            {
                using GlobalNode = ContextToNode<GlobalContext>;
            };
            struct GlobalContextInfo : MainContextInfo
            {
                static void isGlobalContext(detail::GlobalContextTag);
            };
        };

        template<class Context>
        using Node = Cluster<Context>;
    };
}

ARC_MODULE_EXPORT
template<IsNodeHandle MainCluster, IsNodeHandle GlobalNode, class Root = void>
using GraphWithGlobal = Graph<detail::GraphWithGlobal<MainCluster, GlobalNode>, Root>;

} // namespace arc


#endif // INCLUDE_ARC_GLOBAL_HPP
