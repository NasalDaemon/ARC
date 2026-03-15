#ifndef INCLUDE_ARC_INVOKE_METHOD_HPP
#define INCLUDE_ARC_INVOKE_METHOD_HPP

#include "arc/global_context.hpp"
#include "arc/global_trait.hpp"
#include "arc/invoke_method_fwd.hpp"
#include "arc/macros.hpp"
#include "arc/traits/spy.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

namespace arc {

namespace detail {
    template<class Constraints, class Context, class Trait, class Method, class... Args>
    ARC_INLINE constexpr decltype(auto) invokeMethod(auto& node, Method method, Args&&... args)
    {
        Constraints::pre(Context::Info::ContractAssert, args...);
        using R = decltype(node.impl(method, ARC_FWD(args)...));
        if constexpr (std::is_void_v<R>)
        {
            node.impl(method, ARC_FWD(args)...);
        }
        else
        {
            decltype(auto) value = node.impl(method, ARC_FWD(args)...);
            Constraints::post(Context::Info::ContractAssert, value);
            if constexpr (std::is_rvalue_reference_v<decltype(value)>)
                return std::move(value);
            else
                return value;
        }
    }

    template<class Constraints, class Context, class Trait, class Method, class... Args>
    requires (not IsGlobalContext<Context>) and ContextHasGlobalTrait<Context, Global<arc::trait::SpyOnly<Trait>>>
    ARC_INLINE constexpr decltype(auto) invokeMethod(auto& node, Method method, Args&&... args)
    {
        // Enforce pre-contracts before invoking the spy, to ensure the caller is contract-compliant
        Constraints::pre(Context::Info::ContractAssert, args...);

        auto const caller = [&node](auto&&... spyArgs) -> decltype(auto)
        {
            return node.impl(Method{}, static_cast<Args&&>(*static_cast<std::remove_reference_t<Args>*>(std::addressof(spyArgs)))...);
        };

        using R = decltype(node.getGlobal(arc::trait::spyOnly<Trait>).intercept(method, caller, ARC_FWD(args)...));
        if constexpr (std::is_void_v<R>)
        {
            node.getGlobal(arc::trait::spyOnly<Trait>).intercept(method, caller, ARC_FWD(args)...);
        }
        else
        {
            decltype(auto) value = node.getGlobal(arc::trait::spyOnly<Trait>).intercept(method, caller, ARC_FWD(args)...);
            // Enforce post-contracts on the value returned to the caller
            Constraints::post(Context::Info::ContractAssert, value);
            if constexpr (std::is_rvalue_reference_v<decltype(value)>)
                return std::move(value);
            else
                return value;
        }
    }

    template<class Method>
    ARC_INLINE constexpr decltype(auto) normaliseMethodArgs(auto& self, Method method, auto&& f, auto&&... args)
    {
        using Normaliser = decltype(TraitOf<Method>::Meta::SignaturesByTag::impl(self, method, ARC_FWD(args)...))::Normaliser;
        return Normaliser::normalise(ARC_FWD(f), ARC_FWD(args)...);
    }
}

ARC_MODULE_EXPORT
template<class Constraints, class Node, class Method, class... Args>
requires (not IsTraitView<Node>) and requires { typename ContextOf<detail::NodeOf<Node>>; }
ARC_INLINE constexpr decltype(auto) invokeMethod(Node& node, Method method, Args&&... args)
{
    return detail::invokeMethod<Constraints, ContextOf<detail::NodeOf<Node>>, TraitOf<Method>>(node, method, ARC_FWD(args)...);
}

} // namespace arc

#endif // INCLUDE_ARC_INVOKE_METHOD_HPP
