#ifndef INCLUDE_ARC_INVOKE_METHOD_FWD_HPP
#define INCLUDE_ARC_INVOKE_METHOD_FWD_HPP

#include "arc/detail/cast.hpp"
#include "arc/macros.hpp"
#include "arc/trait_view_fwd.hpp"
#include "arc/test/mock_fwd.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

namespace arc {

ARC_MODULE_EXPORT
struct NoConstraints
{
    ARC_INLINE static ARC_IF_MSVC_ELSE(constexpr)(consteval) void pre(auto const&, auto, auto&&...) {}
    ARC_INLINE static ARC_IF_MSVC_ELSE(constexpr)(consteval) void post(auto const&, auto, auto&&) {}
};

ARC_MODULE_EXPORT
template<class Constraints = arc::NoConstraints, class Node, class Method, class... Args>
requires (not IsTraitView<Node>) and requires { typename ContextOf<detail::NodeOf<Node>>; }
ARC_INLINE constexpr decltype(auto) invokeMethod(Node& node, Method method, Args&&... args);

ARC_MODULE_EXPORT
template<class Constraints = arc::NoConstraints, class Node, class Method, class... Args>
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
    using Constraints = NoConstraints;
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
template<bool Const, class Constraints_, class... NormalArgs>
struct InvokeMethod
{
    using Normaliser = NormalInvoker<NormalArgs...>;
    using Constraints = Constraints_;

    ARC_INLINE static constexpr decltype(auto) invoke(auto& node, auto method, auto&&... args)
    {
        return arc::invokeMethod<Constraints>(detail::asConst<Const>(node), method, ARC_FWD(args)...);
    }
};

ARC_MODULE_EXPORT
template<bool Const, class Constraints_, class R, class... NormalArgs>
struct InvokeMethodR
{
    using Normaliser = NormalInvoker<NormalArgs...>;
    using Constraints = Constraints_;

    ARC_INLINE static constexpr R invoke(auto& node, auto method, auto&&... args)
    {
        if constexpr (std::is_void_v<R>)
        {
            arc::invokeMethod<Constraints>(detail::asConst<Const>(node), method, ARC_FWD(args)...);
        }
        else
        {
            using T = decltype(arc::invokeMethod<Constraints>(detail::asConst<Const>(node), method, ARC_FWD(args)...));
            if constexpr (std::is_convertible_v<T, R>)
                return arc::invokeMethod<Constraints>(detail::asConst<Const>(node), method, ARC_FWD(args)...);
            else if constexpr (std::is_same_v<T, test::detail::MockReturn>)
                return arc::invokeMethod<Constraints>(detail::asConst<Const>(node), method, ARC_FWD(args)...).operator R&&();
        }
    }
};

ARC_MODULE_EXPORT
template<typename From, typename To>
concept InvokeConvertibleTo = std::convertible_to<From, To> or std::same_as<From, test::detail::MockReturn>;

ARC_MODULE_EXPORT
template<class T, class ReturnConstraint, class Types>
concept MatchesReturnConstraint = requires { ReturnConstraint::template Return<T, Types>(); };

ARC_MODULE_EXPORT
template<bool Const, class Constraints_, class... NormalArgs>
struct InvokeMethodC
{
    using Normaliser = NormalInvoker<NormalArgs...>;
    using Constraints = Constraints_;

    template<class Node>
    ARC_INLINE static constexpr auto invoke(Node& node, auto method, auto&&... args)
        -> MatchesReturnConstraint<Constraints, typename Node::Types> auto
    {
        return arc::invokeMethod<Constraints>(detail::asConst<Const>(node), method, ARC_FWD(args)...);
    }

    template<class Node>
    requires Constraints::DecltypeAuto
    ARC_INLINE static constexpr auto invoke(Node& node, auto method, auto&&... args)
        -> MatchesReturnConstraint<Constraints, typename Node::Types> decltype(auto)
    {
        return arc::invokeMethod<Constraints>(detail::asConst<Const>(node), method, ARC_FWD(args)...);
    }
};

ARC_MODULE_EXPORT
struct NullSignaturesByTag
{
    static auto impl(auto&&...) -> ::arc::NullNormalInvoker;
};

} // namespace arc

#endif // INCLUDE_ARC_INVOKE_METHOD_FWD_HPP
