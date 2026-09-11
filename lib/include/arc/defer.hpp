#ifndef INCLUDE_ARC_DEFER_HPP
#define INCLUDE_ARC_DEFER_HPP

#include "arc/compiler.hpp"
#include "arc/function.hpp"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <optional>
#include <utility>
#endif

namespace arc {

namespace detail {
    // Named so that the friend redeclaration in Defer repeats the constraint
    // exactly: GCC 16 rejects two inline spellings of it as a redeclaration
    // with different constraints.
    template<class F>
    concept DeferCallable = std::invocable<F> or (compiler < gcc(15));
}

ARC_MODULE_EXPORT
template<detail::DeferCallable F = Function<void()>>
struct [[nodiscard, maybe_unused]] Defer
{
    Defer() = default;
    Defer(auto&&... args)
        : onExit(std::in_place, ARC_FWD(args)...)
    {}
#if ARC_COMPILER_LT(GCC, 15)
    Defer(F&& f)
        : onExit(std::in_place, std::move(f))
    {}
#endif
    template<class F2>
    Defer(Defer<F2> const&) = delete;

    template<class F2>
    Defer(Defer<F2>&& other)
        : onExit(std::exchange(other.onExit, std::nullopt))
    {}

    template<class F2>
    Defer& operator=(Defer<F2>&& other)
    {
        onExit = std::exchange(other.onExit, std::nullopt);
        return *this;
    }

    constexpr bool empty() const { return not onExit.has_value(); }
    constexpr explicit operator bool() const { return not empty(); }

    ~Defer()
    {
        if (onExit)
            (*onExit)();
    }

private:
    template<detail::DeferCallable F2>
    friend struct Defer;

    std::optional<F> onExit;
};

template<class F>
Defer(F) -> Defer<F>;

} // namespace arc


#endif // INCLUDE_ARC_DEFER_HPP
