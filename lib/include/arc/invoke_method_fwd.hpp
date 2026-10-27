#ifndef INCLUDE_ARC_INVOKE_METHOD_FWD_HPP
#define INCLUDE_ARC_INVOKE_METHOD_FWD_HPP

#include "arc/detail/cast.hpp"
#include "arc/cluster_fwd.hpp"
#include "arc/macros.hpp"
#include "arc/static_assert.hpp"
#include "arc/node_fwd.hpp"
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
    static constexpr bool HasPost = false;
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
    ARC_STATIC_ASSERT_INLINE(HasMethods<Node>)("Node does not expose trait methods",
        ARC_SA_NODE(node, Node)(method, arc::typeName<Method>));
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
template<class ReturnType, class T>
ARC_INLINE constexpr ReturnType convertTo(T&& value)
{
    if constexpr (std::is_same_v<test::detail::MockReturn, std::remove_cvref_t<T>>)
        return ARC_FWD(value).template convertTo<ReturnType>();
    else if constexpr (std::is_convertible_v<T, ReturnType>)
        return ARC_FWD(value);
    else
        return ARC_FWD(value).operator ReturnType();
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
    // TODO: Decide how to restrict returning TraitView by default. We return a TraitView in getNode(trait, key::collection).fromHandle(handle)
    template<class T>
    concept MethodReturnable = not (std::is_base_of_v<Node, std::remove_cvref_t<T>> or std::is_base_of_v<Cluster, std::remove_cvref_t<T>>);

    struct InvokeMethodBase
    {
        template<class Self, IsTraitView TraitView>
        ARC_INLINE constexpr auto invoke(this Self const& self, TraitView traitView, auto method, auto&&... args)
            -> MethodReturnable decltype(auto)
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
                else if constexpr (not std::is_reference_v<T> and not std::move_constructible<T>)
                {
                    // Returned by guaranteed elision, so never materialised here for a postcondition to inspect
                    ARC_STATIC_ASSERT_INLINE(not Self::Constraints::HasPost)("Trait method returning a non-movable type cannot have postconditions",
                        (method, arc::typeName<decltype(method)>)(return_type, arc::typeName<T>));
                    return self.invokeImpl(traitView, method, ARC_FWD(args)...);
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
                return self.invokeImpl(traitView, method, ARC_FWD(args)...);
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
        else if constexpr (std::is_same_v<decltype(arc::invokeMethod(detail::asConst<Params.isConst>(traitView), method, ARC_FWD(args)...)), ReturnType>
            and not std::is_rvalue_reference_v<ReturnType>)
        {
            return arc::invokeMethod(detail::asConst<Params.isConst>(traitView), method, ARC_FWD(args)...);
        }
        else
        {
            return convertTo<ReturnType>(arc::invokeMethod(detail::asConst<Params.isConst>(traitView), method, ARC_FWD(args)...));
        }
    }
};

ARC_MODULE_EXPORT
template<typename From, typename To>
concept InvokeConvertibleTo = std::convertible_to<From, To> or std::same_as<From, To> or std::same_as<From, test::detail::MockReturn>;

ARC_MODULE_EXPORT
template<class T, class ReturnConstraint, class Types>
concept MatchesReturnConstraint = requires { ReturnConstraint::template Return<T, Types>(); };

// A trait method's requirement on its implementation, one concept for each way the trait constrains the return type.
// Receiver is the implementation, const for a const method. Return is the constraint, unused for any return type.
ARC_MODULE_EXPORT
template<class Receiver, class Method, class Return, class... Params>
concept ImplementsMethodAnyReturn = requires (Receiver& impl, Method const& method, Params&&... params) {
    impl.impl(method, ARC_FWD(params)...);
};

ARC_MODULE_EXPORT
template<class Receiver, class Method, class To, class... Params>
concept ImplementsMethodConvertibleTo = requires (Receiver& impl, Method const& method, Params&&... params) {
    { impl.impl(method, ARC_FWD(params)...) } -> InvokeConvertibleTo<To>;
};

// Return of arc::ImplementsMethodMatching
ARC_MODULE_EXPORT
template<class ReturnConstraint, class Types_>
struct ReturnConstraintOf
{
    using Constraint = ReturnConstraint;
    using Types = Types_;
};

ARC_MODULE_EXPORT
template<class Receiver, class Method, class Return, class... Params>
concept ImplementsMethodMatching = requires (Receiver& impl, Method const& method, Params&&... params) {
    { impl.impl(method, ARC_FWD(params)...) } -> MatchesReturnConstraint<typename Return::Constraint, typename Return::Types>;
};

namespace detail {
    // Type returned by the implementation, or void when there is none
    template<class Receiver, class Method, class... Params>
    consteval auto implResult()
    {
        if constexpr (ImplementsMethodAnyReturn<Receiver, Method, void, Params...>)
            return std::type_identity<decltype(ARC_DECLVAL(Receiver&).impl(ARC_DECLVAL(Method const&), ARC_DECLVAL(Params&&)...))>{};
        else
            return std::type_identity<void>{};
    }
}

// What ARC_ASSERT_IMPLEMENTS_METHOD needs to tell apart why Impl does not implement a trait method: whether it can be
// called with Params on Impl, or on Impl const if IsConst, and what it returns. Return is as for the requirement.
ARC_MODULE_EXPORT
template<class Impl, bool IsConst, class Method, class Return, class... Params>
struct MethodProbe
{
    using Receiver = std::conditional_t<IsConst, Impl const, Impl>;
    using Result = decltype(detail::implResult<Receiver, Method, Params...>())::type;
    static constexpr bool callable() { return ImplementsMethodAnyReturn<Receiver, Method, Return, Params...>; }
    // Implemented, but not const
    static constexpr bool needsConst() { return IsConst and not callable() and ImplementsMethodAnyReturn<Impl, Method, Return, Params...>; }
};

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
