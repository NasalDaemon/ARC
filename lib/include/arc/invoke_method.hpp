#ifndef INCLUDE_ARC_INVOKE_METHOD_HPP
#define INCLUDE_ARC_INVOKE_METHOD_HPP

#include "arc/global_context.hpp"
#include "arc/global_trait.hpp"
#include "arc/invoke_method_fwd.hpp"
#include "arc/macros.hpp"
#include "arc/traits/spy.hpp"

namespace arc {

namespace detail {
    template<class Context, class Trait, class Method, class... Args>
    ARC_INLINE constexpr decltype(auto) invokeMethod(auto& node, Method method, Args&&... args)
    {
        return node.impl(method, ARC_FWD(args)...);
    }

    template<class Context, class Trait, class Method, class... Args>
    requires (not IsGlobalContext<Context>) and ContextHasGlobalTrait<Context, Global<trait::SpyOnly<Trait>>>
    ARC_INLINE constexpr decltype(auto) invokeMethod(auto& node, Method method, Args&&... args)
    {
        auto const caller = [&node](auto&&... spyArgs) -> decltype(auto)
        {
            return node.impl(Method{}, static_cast<Args&&>(*static_cast<Args*>(std::addressof(spyArgs)))...);
        };

        return node.getGlobal(trait::spyOnly<Trait>).intercept(method, caller, ARC_FWD(args)...);
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
requires (not IsTraitView<Node>) and requires { typename ContextOf<detail::NodeOf<Node>>; }
ARC_INLINE constexpr decltype(auto) invokeMethod(Node& node, Method method, Args&&... args)
{
    return detail::invokeMethod<ContextOf<detail::NodeOf<Node>>, TraitOf<Method>>(node, method, ARC_FWD(args)...);
}

} // namespace arc

#endif // INCLUDE_ARC_INVOKE_METHOD_HPP
