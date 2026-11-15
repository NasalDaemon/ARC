#ifndef INCLUDE_ARC_FUNCTION_HPP
#define INCLUDE_ARC_FUNCTION_HPP

#include "arc/detail/select.hpp"

#include "arc/compiler.hpp"
#include "arc/empty_types.hpp"
#include "arc/macros.hpp"
#include "arc/static_assert.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<class... F>
struct Overload : F...
{
    using F::operator()...;
};

namespace detail {
    // std::invoke_r, which GCC before 15 gets wrong: std::invoke instead, discarding the result when R is void
    template<class R>
    constexpr R invokeR(auto&& f, auto&&... args)
    {
        if constexpr (compiler < gcc(15))
        {
            if constexpr (std::is_void_v<R>)
                std::invoke(ARC_FWD(f), ARC_FWD(args)...);
            else
                return std::invoke(ARC_FWD(f), ARC_FWD(args)...);
        }
        else
            return std::invoke_r<R>(ARC_FWD(f), ARC_FWD(args)...);
    }

    // How a type-erased thunk takes a parameter declared as T: by value when T would be passed in registers anyway
    // (trivially copyable, at most two words), otherwise by reference, so the caller's argument is not moved again
    template<class T>
    using ThunkParam = std::conditional_t<
        std::is_trivially_copyable_v<T> and sizeof(T) <= 2 * sizeof(void*),
        T,
        T&&>;

    template<class T>
    inline constexpr bool isFunctionSig = false;
    template<class R, class... Args>
    inline constexpr bool isFunctionSig<R(Args...)> = true;
}

ARC_MODULE_EXPORT
template<class T>
concept IsFunctionSignature = detail::isFunctionSig<T>;

ARC_MODULE_EXPORT
struct FunctionPolicy
{
    static constexpr bool movable = true;
    bool copyable;
    bool mutableCall;
    bool constCall;
    auto operator<=>(FunctionPolicy const&) const = default;
};

ARC_MODULE_EXPORT
template<class F, class... Args>
concept StatelessInvocable = IsStateless<F> and std::invocable<F, Args...>;

// Lightweight function wrapper that can be used to store any callable object in a single pointer
// It supports mutable and/or immutable calls, and can be made copyable
// Takes a function signature as a template parameter, e.g. `Function<void(int)>`
// Stores a single pointer: `sizeof(Function<void(int)>) == sizeof(void*)`
// Default policy is a move-only object with only a mutable call
// No allocation when storing a stateless callable object (e.g. a lambda with no captures)
ARC_MODULE_EXPORT
template<IsFunctionSignature F, FunctionPolicy = FunctionPolicy{.copyable=false, .mutableCall=true, .constCall=false}>
struct Function;

template<class R, class... Args, FunctionPolicy Policy_>
struct Function<R(Args...), Policy_>
{
    static constexpr FunctionPolicy Policy = Policy_;
    ARC_STATIC_ASSERT(Policy.constCall or Policy.mutableCall)(
        "arc::Function policy must allow const and/or mutable calls",
        (signature, arc::typeName<R(Args...)>));

    Function() = default;
    Function(Function&&) = default;
    Function& operator=(Function&&) = default;

    Function(Function const& other) requires (Policy.copyable)
        : callable{other ? other.callable->copy(other.callable.get()) : nullptr}
    {}
    Function& operator=(Function const& other) requires (Policy.copyable)
    {
        callable.reset(other ? other.callable->copy(other.callable.get()) : nullptr);
        return *this;
    }

    template<std::invocable<Args...> F>
    requires (not std::same_as<std::remove_cvref_t<F>, Function>)
    constexpr Function(F&& f) : callable{makeCallable<std::remove_cvref_t<F>>(ARC_FWD(f))}
    {}

    constexpr R operator()(Args... args) requires (Policy.mutableCall)
    {
        return callable->mutableFunction(callable.get(), std::forward<Args>(args)...);
    }

    constexpr R operator()(Args... args) const requires (Policy.constCall)
    {
        return callable->immutableFunction(callable.get(), std::forward<Args>(args)...);
    }

    constexpr operator bool() const { return callable.get() != nullptr; }

    constexpr void reset() { callable.reset(); }

private:
    struct CallableBase
    {
        using Destroy = void(*)(CallableBase*);
        using MutableFunction = R(*)(CallableBase*, detail::ThunkParam<Args>...);
        using ImmutableFunction = R(*)(CallableBase const*, detail::ThunkParam<Args>...);
        using Copy = CallableBase*(*)(CallableBase const*);

        Destroy destroy;

        ARC_NO_UNIQUE_ADDRESS
        detail::EmptyIf<not Policy.mutableCall, MutableFunction> mutableFunction{};

        ARC_NO_UNIQUE_ADDRESS
        detail::EmptyIf<not Policy.constCall, ImmutableFunction> immutableFunction{};

        ARC_NO_UNIQUE_ADDRESS
        detail::EmptyIf<not Policy.copyable, Copy> copy{};
    };

    template<class F>
    struct Callable : CallableBase
    {
        constexpr explicit Callable(auto&& f)
            : CallableBase{
                .destroy{
                    []([[maybe_unused]] CallableBase* base) -> void {
                        if constexpr (not std::is_empty_v<F>)
                            delete static_cast<Callable*>(base);
                    }
                }}
            , f(ARC_FWD(f))
        {
            if constexpr (Policy.mutableCall)
            {
                this->mutableFunction =
                    [](CallableBase* base, detail::ThunkParam<Args>... args) -> R
                    {
                        return detail::invokeR<R>(static_cast<Callable*>(base)->f, std::forward<Args>(args)...);
                    };
            }
            if constexpr (Policy.constCall)
            {
                this->immutableFunction =
                    [](CallableBase const* base, detail::ThunkParam<Args>... args) -> R
                    {
                        return detail::invokeR<R>(static_cast<Callable const*>(base)->f, std::forward<Args>(args)...);
                    };
            }
            if constexpr (Policy.copyable)
            {
                this->copy =
                    [](CallableBase const* base) -> CallableBase*
                    {
                        if constexpr (std::is_empty_v<F>)
                            // empty callable is statically stored with no state, so we can just return the same pointer
                            return const_cast<CallableBase*>(base);
                        else
                            return new Callable(static_cast<Callable const*>(base)->f);
                    };
            }
        }

        ARC_NO_UNIQUE_ADDRESS F f;
    };

    struct Deleter
    {
        constexpr void operator()(CallableBase* p) const { p->destroy(p); }
    };
    std::unique_ptr<CallableBase, Deleter> callable;

    template<class F>
    requires std::is_empty_v<F>
    static constexpr Callable<F> emptyCallable{F{}};

    template<class F>
    CallableBase* makeCallable(auto&& f)
    {
        if constexpr (std::is_empty_v<F>)
            // shared static instance with no state, which destroy leaves alone
            return const_cast<Callable<F>*>(std::addressof(emptyCallable<F>));
        else
            return new Callable<F>(ARC_FWD(f));
    }
};

// Non-owning reference to any callable object, for a parameter that takes a callback without being a template
// Takes a function signature as a template parameter, e.g. `FunctionRef<void(int)>`
// Stores two pointers: the referenced callable and a function that calls it
// Calling it is one indirect call; inlining the function that takes it lets the compiler inline the callable too
// It does not extend the callable's lifetime: bind it to a parameter, never store one past the call
ARC_MODULE_EXPORT
template<IsFunctionSignature F>
struct FunctionRef;

template<class R, class... Args>
struct FunctionRef<R(Args...)>
{
    template<class F>
    requires (not std::same_as<std::remove_cvref_t<F>, FunctionRef>)
        and std::is_invocable_r_v<R, F&, Args...>
    constexpr FunctionRef(F&& f) noexcept
        : thunk{&call<std::remove_reference_t<F>>}
    {
        if constexpr (std::is_function_v<std::remove_reference_t<F>>)
            target.function = reinterpret_cast<void(*)()>(std::addressof(f));
        else
            target.object = const_cast<void*>(static_cast<void const*>(std::addressof(f)));
    }

    FunctionRef(FunctionRef const&) = default;
    FunctionRef& operator=(FunctionRef const&) = default;

    constexpr R operator()(Args... args) const { return thunk(target, std::forward<Args>(args)...); }

private:
    union Target
    {
        void* object;
        void (*function)();
    };

    template<class F>
    static constexpr R call(Target target, detail::ThunkParam<Args>... args)
    {
        auto&& f = [&]() -> F& {
            if constexpr (std::is_function_v<F>)
                return *reinterpret_cast<F*>(target.function);
            else
                return *static_cast<F*>(target.object);
        }();
        return detail::invokeR<R>(f, std::forward<Args>(args)...);
    }

    Target target;
    R (*thunk)(Target, detail::ThunkParam<Args>...);
};

} // namespace arc

#endif // INCLUDE_ARC_FUNCTION_HPP
