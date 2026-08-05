#ifndef INCLUDE_ARC_FUNCTION_HPP
#define INCLUDE_ARC_FUNCTION_HPP

#include "arc/detail/select.hpp"

#include "arc/compiler.hpp"
#include "arc/empty_types.hpp"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
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
    static_assert(Policy.constCall or Policy.mutableCall);

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

    constexpr R operator()(auto&&... args) requires (Policy.mutableCall)
    {
        return callable->mutableFunction(callable.get(), ARC_FWD(args)...);
    }

    constexpr R operator()(auto&&... args) const requires (Policy.constCall)
    {
        return callable->immutableFunction(callable.get(), ARC_FWD(args)...);
    }

    constexpr operator bool() const { return callable.get() != nullptr; }

    constexpr void reset() { callable.reset(); }

private:
    struct CallableBase
    {
        using Destroy = void(*)(CallableBase*);
        using MutableFunction = R(*)(CallableBase*, Args...);
        using ImmutableFunction = R(*)(CallableBase const*, Args...);
        using Copy = CallableBase*(*)(CallableBase const*);

        Destroy destroy;

        [[no_unique_address]]
        detail::EmptyIf<not Policy.mutableCall, MutableFunction> mutableFunction{};

        [[no_unique_address]]
        detail::EmptyIf<not Policy.constCall, ImmutableFunction> immutableFunction{};

        [[no_unique_address]]
        detail::EmptyIf<not Policy.copyable, Copy> copy{};
    };

    template<class F>
    struct Callable : CallableBase
    {
        constexpr explicit Callable(auto&& f)
            : CallableBase{
                .destroy{
                    [](CallableBase* base) -> void {
                        if constexpr (not std::is_empty_v<F>)
                            delete static_cast<Callable*>(base);
                    }
                }}
            , f(ARC_FWD(f))
        {
            if constexpr (Policy.mutableCall)
            {
                this->mutableFunction =
                    [](CallableBase* base, Args... args) -> R
                    {
                        if constexpr (compiler < gcc(15))
                            return std::invoke(static_cast<Callable*>(base)->f, ARC_FWD(args)...);
                        else
                            return std::invoke_r<R>(static_cast<Callable*>(base)->f, ARC_FWD(args)...);
                    };
            }
            if constexpr (Policy.constCall)
            {
                this->immutableFunction =
                    [](CallableBase const* base, Args... args) -> R
                    {
                        if constexpr (compiler < gcc(15))
                            return std::invoke(static_cast<Callable const*>(base)->f, ARC_FWD(args)...);
                        else
                            return std::invoke_r<R>(static_cast<Callable const*>(base)->f, ARC_FWD(args)...);
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

        [[no_unique_address]] F f;
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
            return emptyCallable<F>.copy(std::addressof(emptyCallable<F>));
        else
            return new Callable<F>(ARC_FWD(f));
    }
};

} // namespace arc

#endif // INCLUDE_ARC_FUNCTION_HPP
