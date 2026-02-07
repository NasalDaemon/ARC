#ifndef INCLUDE_ARC_INVOKE_METHOD_FWD_HPP
#define INCLUDE_ARC_INVOKE_METHOD_FWD_HPP

#include "arc/detail/cast.hpp"
#include "arc/macros.hpp"
#include "arc/trait_view_fwd.hpp"

namespace arc {

ARC_MODULE_EXPORT
template<class Node, class Method, class... Args>
requires (not IsTraitView<Node>) and requires { typename ContextOf<detail::NodeOf<Node>>; }
ARC_INLINE constexpr decltype(auto) invokeMethod(Node& node, Method method, Args&&... args);

ARC_MODULE_EXPORT
template<class Node, class Method, class... Args>
ARC_INLINE constexpr decltype(auto) invokeMethod(Node& node, Method method, Args&&... args)
{
    return node.impl(method, ARC_FWD(args)...);
}

ARC_MODULE_EXPORT
struct NullNormalInvoker
{
    struct Normaliser
    {
        ARC_INLINE static constexpr decltype(auto) normalise(auto&& f, auto&&... args)
        {
            return ARC_FWD(f)(ARC_FWD(args)...);
        }
    };
};

template<class... NormalArgs>
struct NormalInvoker
{
    template<std::invocable<NormalArgs...> F>
    ARC_INLINE static constexpr decltype(auto) normalise(F&& f, NormalArgs... args)
    {
        return ARC_FWD(f)(ARC_FWD(args)...);
    }
};

ARC_MODULE_EXPORT
template<bool Const, class... NormalArgs>
struct InvokeMethod
{
    using Normaliser = NormalInvoker<NormalArgs...>;

    ARC_INLINE static constexpr decltype(auto) invoke(auto& node, auto method, auto&&... args)
    {
        return arc::invokeMethod(detail::asConst<Const>(node), method, ARC_FWD(args)...);
    }
};

ARC_MODULE_EXPORT
template<bool Const, class Return, class... NormalArgs>
struct InvokeMethodR
{
    using Normaliser = NormalInvoker<NormalArgs...>;

    ARC_INLINE static constexpr Return invoke(auto& node, auto method, auto&&... args)
    {
        if constexpr (std::is_void_v<Return>)
            arc::invokeMethod(detail::asConst<Const>(node), method, ARC_FWD(args)...);
        else
            return arc::invokeMethod(detail::asConst<Const>(node), method, ARC_FWD(args)...);
    }
};

ARC_MODULE_EXPORT
template<class T, class ReturnConstraint, class Types>
concept MatchesReturnConstraint = requires { ReturnConstraint::template Return<T, Types>(); };

ARC_MODULE_EXPORT
template<bool Const, class ReturnConstraint, class... NormalArgs>
struct InvokeMethodC
{
    using Normaliser = NormalInvoker<NormalArgs...>;

    template<class Node>
    ARC_INLINE static constexpr auto invoke(Node& node, auto method, auto&&... args)
        -> MatchesReturnConstraint<ReturnConstraint, typename Node::Types> auto
    {
        return arc::invokeMethod(detail::asConst<Const>(node), method, ARC_FWD(args)...);
    }

    template<class Node>
    ARC_INLINE static constexpr auto invoke(Node& node, auto method, auto&&... args)
        -> MatchesReturnConstraint<ReturnConstraint, typename Node::Types> decltype(auto)
        requires ReturnConstraint::DecltypeAuto
    {
        return arc::invokeMethod(detail::asConst<Const>(node), method, ARC_FWD(args)...);
    }
};

} // namespace arc

#endif // INCLUDE_ARC_INVOKE_METHOD_FWD_HPP
