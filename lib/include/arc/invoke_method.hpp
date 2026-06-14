#ifndef INCLUDE_ARC_INVOKE_METHOD_HPP
#define INCLUDE_ARC_INVOKE_METHOD_HPP

#include "arc/context_fwd.hpp"
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

    // Wrapper around the spy-dispatch callable that surfaces compile-time
    // identity of the intercepted node so spies can read it off `impl_fn`.
    template<class ContextT, class Inner>
    struct SpyCaller
    {
        using Context = ContextT;
        // The intercepted node's user-facing handle — the same type a cluster
        // definition names via `foo = node::Foo` (or its higher-order wrapper).
        // Read directly off the context's `NodeHandle` typedef.
        using NodeHandle = ContextT::NodeHandle;

        Inner inner;

        ARC_INLINE constexpr decltype(auto) operator()(auto&&... args) const
        {
            return inner(ARC_FWD(args)...);
        }
    };

    template<class Context, class View, class Node, class Method, class... Args>
    ARC_INLINE constexpr decltype(auto) invokeMethodWithSpy(std::nullptr_t/* spy */, View&& view, Node& node, Method method, Args&&... args)
    {
        static_assert(CanInvokeMethod<Node, Method, Args...>);
        static_assert(HasMethods<View>);
        if constexpr (ImplementsMethod<Node, Method, Args...>)
            return node.impl(method, ARC_FWD(args)...);
        else
            return invokeMethodDefault(view, method, ARC_FWD(args)...);
    }

    template<class Context, class SpyTraitView, class View, class Node, class Method, class... Args>
    ARC_INLINE constexpr decltype(auto) invokeMethodWithSpy(SpyTraitView spy, View&& view, Node& node, Method method, Args&&... args)
    {
        static_assert(CanInvokeMethod<Node, Method, Args...>);
        static_assert(HasMethods<View>);
        auto caller = [&](auto&&... spyArgs) -> decltype(auto)
        {
            if constexpr (ImplementsMethod<Node, Method, Args...>)
                return node.impl(Method{}, static_cast<Args&&>(*static_cast<std::remove_reference_t<Args>*>(std::addressof(spyArgs)))...);
            else
                return invokeMethodDefault(view, Method{}, static_cast<Args&&>(*static_cast<std::remove_reference_t<Args>*>(std::addressof(spyArgs)))...);
        };

        return spy.intercept(method, SpyCaller<Context, decltype(caller)>(std::move(caller)), ARC_FWD(args)...);
    }

    template<class Context, class Trait, class Node>
    ARC_INLINE constexpr auto getSpy(Node& node)
    {
        if constexpr (IsSpyTrait<Trait>)
            return nullptr;
        else if constexpr (IsGlobalContext<Context> and HasTrait<ContextToNode<Context>, arc::SpyOnly<Trait>>)
            return nullptr;
        else if constexpr (ContextHasGlobalTrait<Context, Global<arc::SpyOnly<Trait>>>)
            return node.getGlobal(arc::spyOnly<Trait>);
        else
            return nullptr;
    }

    template<class Context, class Trait, class Node, class Method, class... Args>
    ARC_INLINE constexpr decltype(auto) invokeMethod(Node& node, Method method, Args&&... args)
    {
        static_assert(CanInvokeMethod<Node, Method, Args...>);
        return invokeMethodWithSpy<Context>(getSpy<Context, Trait>(node), node, node, method, ARC_FWD(args)...);
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
