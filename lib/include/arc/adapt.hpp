#ifndef INCLUDE_ARC_ADAPT_HPP
#define INCLUDE_ARC_ADAPT_HPP

#include "arc/detail/cast.hpp"
#include "arc/args.hpp"
#include "arc/cluster.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"
#include "arc/virtual.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <memory>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<IsNodeHandle Target, IsNodeHandle Facade>
struct Adapt
{
    template<IsVirtualContext Context>
    struct Node : arc::Cluster
    {
        struct Facade_;
        struct Target_;

        template<IsTrait T>
        static ResolvedLink<Facade_, T> resolveLink(T, LinkPriorityMin);

        constexpr auto* asInterface(this auto& self) { return std::addressof(self.facade); }

        struct Facade_ : arc::Context<Node, Facade>
        {
            static constexpr std::size_t Depth = Context::Depth;

            template<class Trait>
            static ResolvedLink<Target_, Trait> resolveLink(Trait, LinkPriorityMin);

            static Facade_ isVirtualContext(detail::IsVirtualContextTag);

            template<IsNodeHandle T, class N>
            requires (not std::is_const_v<N>)
            static constexpr auto exchangeImpl(N& facade, auto&&... args)
            {
                auto memPtr = ARC_MEM_PTR(Node, facade);
                auto& cluster = memPtr.getClassFromMember(facade);
                return Context::template exchangeImpl<T>(cluster, ARC_FWD(args)...);
            }
        };

        struct Target_ : arc::Context<Node, Target>
        {
            static constexpr std::size_t Depth = Context::Depth;

            template<class Trait>
            requires detail::HasLink<Context, Trait>
            static ResolvedLink<Context, Trait> resolveLink(Trait, LinkPriorityMin);
        };

        ARC_NODE(Target_, target)

        detail::ToVirtualNodeImpl<Facade, Facade_> facade{};
        friend consteval auto getNodePointer(arc::AdlTag<Facade_>) { return ARC_MEM_PTR(Node, facade); }
        static_assert(IsInterface<decltype(facade)>);

        template<class... Ts>
        requires (... and not IsArgs<Ts>)
        explicit constexpr Node(Ts&&... args) : target{ARC_FWD(args)...} {}

        template<class... FArgs, class... TArgs>
        explicit constexpr Node(Args<Target, TArgs...> const& targs, Args<Facade, FArgs...> const& fargs)
            : target{targs.template get<TArgs>()...}
            , facade{fargs.template get<FArgs>()...}
        {}

        constexpr void visit(this auto& self, auto&& visitor)
        {
            self.target.visit(visitor);
            self.facade.visit(visitor);
        }
    };
};

ARC_MODULE_EXPORT
template<IsNodeHandle Target, IsNodeHandle Facade>
inline constexpr std::in_place_type_t<arc::Adapt<Target, Facade>> adapt{};

}

#endif // INCLUDE_ARC_ADAPT_HPP
