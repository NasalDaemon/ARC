#ifndef INCLUDE_ARC_INVOKE_METHOD_HPP
#define INCLUDE_ARC_INVOKE_METHOD_HPP

#include "arc/global_context.hpp"
#include "arc/global_trait.hpp"
#include "arc/invoke_method_fwd.hpp"
#include "arc/macros.hpp"
#include "arc/trait_view_fwd.hpp"
#include "arc/traits/spy.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

namespace arc {

namespace detail {

    template<class View, class Node, class Method, class... Args>
    ARC_INLINE constexpr decltype(auto) invokeMethodWithGlobal(std::nullptr_t, View&& view, Node& node, Method method, Args&&... args)
    {
        static_assert(CanInvokeMethod<Node, Method, Args...>);
        static_assert(HasMethods<View>);
        if constexpr (ImplementsMethod<Node, Method, Args...>)
            return node.impl(method, ARC_FWD(args)...);
        else
            return invokeMethodDefault(view, method, ARC_FWD(args)...);
    }

    template<class GlobalTraitView, class View, class Node, class Method, class... Args>
    ARC_INLINE constexpr decltype(auto) invokeMethodWithGlobal(GlobalTraitView global, View&& view, Node& node, Method method, Args&&... args)
    {
        static_assert(CanInvokeMethod<Node, Method, Args...>);
        static_assert(HasMethods<View>);
        auto const caller = [&](auto&&... spyArgs) -> decltype(auto)
        {
            if constexpr (ImplementsMethod<Node, Method, Args...>)
                return node.impl(Method{}, static_cast<Args&&>(*static_cast<std::remove_reference_t<Args>*>(std::addressof(spyArgs)))...);
            else
                return invokeMethodDefault(view, Method{}, static_cast<Args&&>(*static_cast<std::remove_reference_t<Args>*>(std::addressof(spyArgs)))...);
        };

        return global.intercept(method, caller, ARC_FWD(args)...);
    }

    template<class Context, class Trait, class Node>
    ARC_INLINE constexpr auto getGlobal(Node& node)
    {
        if constexpr (not IsGlobalContext<Context> and ContextHasGlobalTrait<Context, Global<arc::trait::SpyOnly<Trait>>>)
            return node.getGlobal(arc::trait::spyOnly<Trait>);
        else
            return nullptr;
    }

    template<class Context, class Trait, class Node, class Method, class... Args>
    ARC_INLINE constexpr decltype(auto) invokeMethod(Node& node, Method method, Args&&... args)
    {
        static_assert(CanInvokeMethod<Node, Method, Args...>);
        return invokeMethodWithGlobal(getGlobal<Context, Trait>(node), node, node, method, ARC_FWD(args)...);
    }

    template<class Method>
    ARC_INLINE constexpr decltype(auto) normaliseMethodArgs(auto& self, Method method, auto&& f, auto&&... args)
    {
        using Normaliser = decltype(TraitOf<Method>::Meta::SignaturesByTag::impl(self, method, ARC_FWD(args)...))::Normaliser;
        return Normaliser::normalise(ARC_FWD(f), ARC_FWD(args)...);
    }
}

ARC_MODULE_EXPORT
template<class Node, class Method, class... Args>
requires (not IsTraitView<Node>) and requires { typename ContextOf<detail::NodeOf<Node>>; } and CanInvokeMethod<Node, Method, Args...>
ARC_INLINE constexpr decltype(auto) invokeMethod(Node& node, Method method, Args&&... args)
{
    return detail::invokeMethod<ContextOf<detail::NodeOf<Node>>, TraitOf<Method>>(node, method, ARC_FWD(args)...);
}

} // namespace arc

#endif // INCLUDE_ARC_INVOKE_METHOD_HPP
