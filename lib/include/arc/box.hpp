#ifndef INCLUDE_ARC_BOX_HPP
#define INCLUDE_ARC_BOX_HPP

#include "arc/detail/cast.hpp"
#include "arc/adapt.hpp"
#include "arc/cluster.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/node.hpp"
#include "arc/virtual.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <memory>
#endif

namespace arc {

namespace detail {

    template<class OuterNode>
    struct OuterNodeProxy
    {
        template<class Context>
        struct Node : arc::Node
        {
            template<class Trait>
            struct WithTrait;

            template<class Trait>
            // Types not used as this is behind a facade which already concretises the types
            using Resolver = ResolvedTrait<WithTrait<Trait>, EmptyTypes>;

            using Traits = arc::TraitsTemplate<Node, Resolver>;

            explicit constexpr Node(OuterNode* outerNode) : outerNode(outerNode) {}

            OuterNode* outerNode;
        };
    };

    template<class OuterNode>
    template<class Context>
    template<class Trait>
    struct OuterNodeProxy<OuterNode>::Node<Context>::WithTrait : Node
    {
        template<class Self>
        ARC_INLINE constexpr decltype(auto) impl(this Self& self, auto&&... args)
        {
            return std::forward_like<Self&>(*self.outerNode).getNode(Trait{}).impl(ARC_FWD(args)...);
        }
    };

    template<IsNodeHandle Main, IsNodeHandle InFacade, class OutFacade, IsInterface... OutInterfaces>
    requires (IsNodeHandle<OutFacade> and sizeof...(OutInterfaces) > 0)
          or (std::is_void_v<OutFacade> and sizeof...(OutInterfaces) == 0)
    struct Boxed
    {
        template<IsRootContext Context>
        struct Node : arc::Cluster
        {
            struct Main_;
            struct InFacade_;
            struct OutVirtual_;

            constexpr auto* asInterface(this auto& self) { return std::addressof(self.inFacade); }

            struct Main_ : arc::Context<Node, Main>
            {
                template<class Trait>
                static ResolvedLink<OutVirtual_, Trait> resolveLink(Trait, LinkPriorityMin);
            };

            struct InFacade_ : arc::Context<Node, InFacade>
            {
                template<class Trait>
                static ResolvedLink<Main_, Trait> resolveLink(Trait, LinkPriorityMin);
            };

            struct OutVirtual_ : arc::Context<Node, arc::Virtual<OutInterfaces...>>
            {};

            ARC_NODE(Main_, main)

            detail::ToVirtualNodeImpl<InFacade, InFacade_> inFacade{};
            friend consteval auto getNodePointer(arc::AdlTag<InFacade_>) { return ARC_MEM_PTR(Node, inFacade); }
            static_assert(IsInterface<decltype(inFacade)>);

            ARC_NODE(OutVirtual_, outVirtual)

            template<class OutNode, class... Ts>
            requires (... and not IsArgs<Ts>)
            constexpr explicit Node(OutNode* outNode, Ts&&... args)
                : main{ARC_FWD(args)...}
                , outVirtual{std::in_place_type<Adapt<OutNode>>, outNode}
            {}

            template<class OutNode, class... MArgs, class... IArgs>
            explicit constexpr Node(OutNode* outNode, Args<Main, MArgs...> const& margs, Args<InFacade, IArgs...> const& iargs)
                : main{margs.template get<MArgs>()...}
                , inFacade{iargs.template get<IArgs>()...}
                , outVirtual{std::in_place_type<Adapt<OutNode>>, outNode}
            {}


            template<class OutNode>
            void setVirtualHost(OutNode* outNode)
            {
                outVirtual->template emplace<Adapt<OutNode>>(outNode);
            }

            constexpr void visit(this auto& self, auto&& visitor)
            {
                self.main.visit(visitor);
                self.inFacade.visit(visitor);
                self.outVirtual.visit(visitor);
            }

        private:
            template<class OutNode>
            static constexpr auto getAdapt() -> arc::Adapt<OuterNodeProxy<OutNode>, OutFacade>;

            template<class OutNode>
            requires std::is_void_v<OutFacade>
            static constexpr auto getAdapt() -> void;

            template<class OutNode>
            using Adapt = decltype(getAdapt<OutNode>());
        };
    };

} // namespace detail

ARC_MODULE_EXPORT
template<IsRootContext Root, IsNodeHandle Main, IsNodeHandle InFacade, class OutFacade = void, IsInterface... OutInterfaces>
struct BoxWithRoot
{
    using Graph = arc::Graph<detail::Boxed<Main, InFacade, OutFacade, OutInterfaces...>, Root>;

    template<IsVirtualContext>
    using Node = Graph;

    template<IsRootContext NewRoot = NullContext>
    using WithRoot = BoxWithRoot<NewRoot, Main, InFacade, OutFacade, OutInterfaces...>;
};

ARC_MODULE_EXPORT
template<IsRootContext Root, IsNodeHandle Main, IsNodeHandle InFacade, class OutFacade = void, IsInterface... OutInterfaces>
inline constexpr std::in_place_type_t<BoxWithRoot<Root, Main, InFacade, OutFacade, OutInterfaces...>> boxWithRoot{};

ARC_MODULE_EXPORT
template<IsNodeHandle Main, IsNodeHandle InFacade, class OutFacade = void, IsInterface... OutInterfaces>
struct Box
{
    using Graph = arc::Graph<detail::Boxed<Main, InFacade, OutFacade, OutInterfaces...>>;

    template<IsVirtualContext>
    using Node = Graph;

    template<IsRootContext Root = NullContext>
    using WithRoot = BoxWithRoot<Root, Main, InFacade, OutFacade, OutInterfaces...>;
};

namespace detail {
    template<IsRootContext Root, IsNodeHandle Main, IsNodeHandle InFacade, class OutFacade, IsInterface... OutInterfaces>
    constexpr bool injectVirtualHost<BoxWithRoot<Root, Main, InFacade, OutFacade, OutInterfaces...>> = true;
    template<IsNodeHandle Main, IsNodeHandle InFacade, class OutFacade, IsInterface... OutInterfaces>
    constexpr bool injectVirtualHost<Box<Main, InFacade, OutFacade, OutInterfaces...>> = true;
}

ARC_MODULE_EXPORT
template<IsNodeHandle Main, IsNodeHandle InFacade, class OutFacade = void, IsInterface... OutInterfaces>
inline constexpr std::in_place_type_t<Box<Main, InFacade, OutFacade, OutInterfaces...>> box{};

}

#endif // INCLUDE_ARC_BOX_HPP
