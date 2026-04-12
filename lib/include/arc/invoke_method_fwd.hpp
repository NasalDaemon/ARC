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
    ARC_INLINE static constexpr void pre(auto const&, auto, auto&&...) {}
    ARC_INLINE static constexpr void post(auto const&, auto, auto&&) {}
};

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
    using Constraints = NoConstraints;
};

ARC_MODULE_EXPORT
struct NullSignaturesByTag
{
    static auto impl(auto&&...) -> ::arc::NullNormalInvoker;
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

namespace detail {

    struct InvokeMethodBase
    {
        template<class Self, IsTraitView TraitView>
        ARC_INLINE constexpr decltype(auto) invoke(this Self const& self, TraitView traitView, auto method, auto&&... args)
        {
            using Context = ContextOf<typename TraitView::Node>;
            if constexpr (Context::Info::ContractAssert.enabled)
                Self::Constraints::pre(Context::Info::ContractAssert, traitView, args...);

            using T = decltype(self.invokeImpl(traitView, method, ARC_FWD(args)...));
            if constexpr (std::is_void_v<T>)
            {
                self.invokeImpl(traitView, method, ARC_FWD(args)...);

                if constexpr (Context::Info::ContractAssert.enabled)
                    Self::Constraints::post(Context::Info::ContractAssert, traitView, nullptr);
            }
            else
            {
                decltype(auto) value = self.invokeImpl(traitView, method, ARC_FWD(args)...);

                if constexpr (Context::Info::ContractAssert.enabled)
                    Self::Constraints::post(Context::Info::ContractAssert, traitView, value);

                if constexpr (std::is_rvalue_reference_v<T>)
                    return std::move(value);
                else
                    return value;
            }

        }
    };

} // namespace detail

ARC_MODULE_EXPORT
template<bool Const, class Constraints_, class... NormalArgs>
struct InvokeMethod : detail::InvokeMethodBase
{
    using Normaliser = NormalInvoker<NormalArgs...>;
    using Constraints = Constraints_;

private:
    friend detail::InvokeMethodBase;

    ARC_INLINE static constexpr decltype(auto) invokeImpl(IsTraitView auto traitView, auto method, auto&&... args)
    {
        return arc::invokeMethod(detail::asConst<Const>(traitView), method, ARC_FWD(args)...);
    }
};

ARC_MODULE_EXPORT
template<bool Const, class Constraints_, class R, class... NormalArgs>
struct InvokeMethodR : detail::InvokeMethodBase
{
    using Normaliser = NormalInvoker<NormalArgs...>;
    using Constraints = Constraints_;

private:
    friend detail::InvokeMethodBase;

    ARC_INLINE static constexpr R invokeImpl(IsTraitView auto traitView, auto method, auto&&... args)
    {
        if constexpr (std::is_void_v<R>)
        {
            arc::invokeMethod(detail::asConst<Const>(traitView), method, ARC_FWD(args)...);
        }
        else
        {
            using T = decltype(arc::invokeMethod(detail::asConst<Const>(traitView), method, ARC_FWD(args)...));
            if constexpr (std::is_convertible_v<T, R>)
                return arc::invokeMethod(detail::asConst<Const>(traitView), method, ARC_FWD(args)...);
            else if constexpr (std::is_same_v<T, test::detail::MockReturn>)
                return arc::invokeMethod(detail::asConst<Const>(traitView), method, ARC_FWD(args)...).operator R&&();
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
struct InvokeMethodC : detail::InvokeMethodBase
{
    using Normaliser = NormalInvoker<NormalArgs...>;
    using Constraints = Constraints_;

private:
    friend detail::InvokeMethodBase;

    template<IsTraitView TraitView>
    ARC_INLINE static constexpr auto invokeImpl(TraitView traitView, auto method, auto&&... args)
        -> MatchesReturnConstraint<Constraints, typename TraitView::Types> auto
    {
        return arc::invokeMethod(detail::asConst<Const>(traitView), method, ARC_FWD(args)...);
    }

    template<IsTraitView TraitView>
    requires Constraints::DecltypeAuto
    ARC_INLINE static constexpr auto invokeImpl(TraitView traitView, auto method, auto&&... args)
        -> MatchesReturnConstraint<Constraints, typename TraitView::Types> decltype(auto)
    {
        return arc::invokeMethod(detail::asConst<Const>(traitView), method, ARC_FWD(args)...);
    }
};

} // namespace arc

#endif // INCLUDE_ARC_INVOKE_METHOD_FWD_HPP
