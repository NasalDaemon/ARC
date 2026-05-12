#ifndef INCLUDE_ARC_INVOKE_METHOD_FWD_HPP
#define INCLUDE_ARC_INVOKE_METHOD_FWD_HPP

#include "arc/detail/cast.hpp"
#include "arc/macros.hpp"
#include "arc/trait.hpp"
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
    struct NoProtoSnap {};
    ARC_INLINE static constexpr NoProtoSnap preProto(auto const&, auto, auto&&...) { return {}; }
    ARC_INLINE static constexpr void postProto(auto const&, auto, auto const&, NoProtoSnap&) {}
};

ARC_MODULE_EXPORT
template<class Interface, class Method, class... Args>
concept ImplementsMethod = requires(Interface& interface, Method method, Args&&... args) {
    interface.impl(method, ARC_FWD(args)...);
};

ARC_MODULE_EXPORT
template<class Interface, class Method, class... Args>
concept HasDefaultImpl = decltype(TraitOf<Method>::Meta::SignaturesByTag::impl(std::declval<Interface&>(), Method{}, std::declval<Args>()...))::HasDefault;

ARC_MODULE_EXPORT
template<class Interface, class Method, class... Args>
concept CanInvokeMethod = ImplementsMethod<Interface, Method, Args...> or HasDefaultImpl<Interface, Method, Args...>;

ARC_MODULE_EXPORT
template<class Node, class Method, class... Args>
requires HasDefaultImpl<Node, Method, Args...>
ARC_INLINE constexpr decltype(auto) invokeMethodDefault(Node& node, Method method, Args&&... args)
{
    static_assert(HasMethods<Node>);
    return TraitOf<Method>::Meta::DefaultImpl::defaultImpl(node, method, ARC_FWD(args)...);
}

ARC_MODULE_EXPORT
template<class Node, class Method, class... Args>
requires (not IsTraitView<Node>) and requires { typename ContextOf<detail::NodeOf<Node>>; } and CanInvokeMethod<Node, Method, Args...>
ARC_INLINE constexpr decltype(auto) invokeMethod(Node& node, Method method, Args&&... args);

ARC_MODULE_EXPORT
template<class Node, class Method, class... Args>
requires CanInvokeMethod<Node, Method, Args...>
ARC_INLINE constexpr decltype(auto) invokeMethod(Node& node, Method method, Args&&... args)
{
    if constexpr (ImplementsMethod<Node, Method, Args...>)
        return node.impl(method, ARC_FWD(args)...);
    else
        return invokeMethodDefault(node, method, ARC_FWD(args)...);
}

ARC_MODULE_EXPORT
struct InvokerParams
{
    bool isConst = false;
    bool hasDefault = false;

    auto operator<=>(InvokerParams const&) const = default;
};

ARC_MODULE_EXPORT
struct NullNormalInvoker
{
    static constexpr bool HasDefault = false;

    struct Normaliser
    {
        ARC_INLINE static constexpr decltype(auto) normalise(auto&& f, auto&&... args)
        {
            return ARC_FWD(f)(std::false_type{}, ARC_FWD(args)...);
        }
    };
    using Constraints = NoConstraints;
};

ARC_MODULE_EXPORT
struct NullSignaturesByTag
{
    static auto impl(auto&&...) -> ::arc::NullNormalInvoker;
};

template<bool HasDefault, class... NormalArgs>
struct NormalInvoker
{
    template<std::invocable<std::bool_constant<HasDefault>, NormalArgs...> F>
    ARC_INLINE static constexpr decltype(auto) normalise(F&& f, NormalArgs... args)
    {
        return ARC_FWD(f)(std::bool_constant<HasDefault>{}, ARC_FWD(args)...);
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
            {
                auto const tvNarrow = traitView.asTrait(traitOf(method), std::true_type{});
                auto snap = Self::Constraints::preProto(Context::Info::ContractAssert, tvNarrow, args...);
                Self::Constraints::pre(Context::Info::ContractAssert, tvNarrow, args...);

                using T = decltype(self.invokeImpl(traitView, method, ARC_FWD(args)...));
                if constexpr (std::is_void_v<T>)
                {
                    self.invokeImpl(traitView, method, ARC_FWD(args)...);
                    Self::Constraints::post(Context::Info::ContractAssert, tvNarrow, nullptr);
                    Self::Constraints::postProto(Context::Info::ContractAssert, tvNarrow, nullptr, snap);
                }
                else
                {
                    decltype(auto) value = self.invokeImpl(traitView, method, ARC_FWD(args)...);
                    Self::Constraints::post(Context::Info::ContractAssert, tvNarrow, value);
                    Self::Constraints::postProto(Context::Info::ContractAssert, tvNarrow, value, snap);
                    if constexpr (std::is_rvalue_reference_v<T>)
                        return std::move(value);
                    else
                        return value;
                }
            }
            else
            {
                using T = decltype(self.invokeImpl(traitView, method, ARC_FWD(args)...));
                if constexpr (std::is_void_v<T>)
                    self.invokeImpl(traitView, method, ARC_FWD(args)...);
                else
                {
                    decltype(auto) value = self.invokeImpl(traitView, method, ARC_FWD(args)...);
                    if constexpr (std::is_rvalue_reference_v<T>)
                        return std::move(value);
                    else
                        return value;
                }
            }
        }
    };

} // namespace detail

ARC_MODULE_EXPORT
template<InvokerParams Params, class Constraints_, class... NormalArgs>
struct InvokeMethod : detail::InvokeMethodBase
{
    static constexpr bool HasDefault = Params.hasDefault;
    using Normaliser = NormalInvoker<Params.hasDefault, NormalArgs...>;
    using Constraints = Constraints_;

private:
    friend detail::InvokeMethodBase;

    ARC_INLINE static constexpr decltype(auto) invokeImpl(IsTraitView auto traitView, auto method, auto&&... args)
    {
        return arc::invokeMethod(detail::asConst<Params.isConst>(traitView), method, ARC_FWD(args)...);
    }
};

ARC_MODULE_EXPORT
template<InvokerParams Params, class Constraints_, class ReturnType_, class... NormalArgs>
struct InvokeMethodR : detail::InvokeMethodBase
{
    static constexpr bool HasDefault = Params.hasDefault;
    using ReturnType = ReturnType_;
    using Normaliser = NormalInvoker<Params.hasDefault, NormalArgs...>;
    using Constraints = Constraints_;

private:
    friend detail::InvokeMethodBase;

    ARC_INLINE static constexpr ReturnType invokeImpl(IsTraitView auto traitView, auto method, auto&&... args)
    {
        if constexpr (std::is_void_v<ReturnType>)
        {
            arc::invokeMethod(detail::asConst<Params.isConst>(traitView), method, ARC_FWD(args)...);
        }
        else
        {
            using T = decltype(arc::invokeMethod(detail::asConst<Params.isConst>(traitView), method, ARC_FWD(args)...));
            if constexpr (std::is_convertible_v<T, ReturnType>)
                return arc::invokeMethod(detail::asConst<Params.isConst>(traitView), method, ARC_FWD(args)...);
            else if constexpr (std::is_same_v<T, test::detail::MockReturn>)
                return arc::invokeMethod(detail::asConst<Params.isConst>(traitView), method, ARC_FWD(args)...).operator ReturnType&&();
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
template<InvokerParams Params, class Constraints_, class... NormalArgs>
struct InvokeMethodC : detail::InvokeMethodBase
{
    static constexpr bool HasDefault = Params.hasDefault;
    using Normaliser = NormalInvoker<Params.hasDefault, NormalArgs...>;
    using Constraints = Constraints_;

private:
    friend detail::InvokeMethodBase;

    template<IsTraitView TraitView>
    ARC_INLINE static constexpr auto invokeImpl(TraitView traitView, auto method, auto&&... args)
        -> MatchesReturnConstraint<Constraints, typename TraitView::Types> auto
    {
        return arc::invokeMethod(detail::asConst<Params.isConst>(traitView), method, ARC_FWD(args)...);
    }

    template<IsTraitView TraitView>
    requires Constraints::DecltypeAuto
    ARC_INLINE static constexpr auto invokeImpl(TraitView traitView, auto method, auto&&... args)
        -> MatchesReturnConstraint<Constraints, typename TraitView::Types> decltype(auto)
    {
        return arc::invokeMethod(detail::asConst<Params.isConst>(traitView), method, ARC_FWD(args)...);
    }
};

} // namespace arc

#endif // INCLUDE_ARC_INVOKE_METHOD_FWD_HPP
