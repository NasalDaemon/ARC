#ifndef INCLUDE_ARC_FACTORY_HPP
#define INCLUDE_ARC_FACTORY_HPP

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <utility>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<class Type>
struct Constructor
{
    using type = Type;

    template<class... Args>
    requires std::constructible_from<Type, Args...>
    ARC_INLINE constexpr Type operator()(Args&&... args) const
    {
        return Type{ARC_FWD(args)...};
    }
};

namespace detail {
    struct NonRelocatable
    {
        NonRelocatable() = default;
        NonRelocatable(NonRelocatable&&) = delete;
        NonRelocatable(NonRelocatable const&) = delete;
    };
}

ARC_MODULE_EXPORT
template<class F>
struct Emplace
{
    template<class Type>
    ARC_INLINE explicit constexpr operator Type() &&
    {
        static_assert(std::is_same_v<Type, std::invoke_result_t<F, Constructor<Type>>>);
        return std::move(factory)(Constructor<Type>{});
    }

    [[no_unique_address]] F factory;
    // Ensure that Emplace is never relocated to avoid dangling references
    [[no_unique_address]] detail::NonRelocatable nonRelocatable{};
};

template<class F>
Emplace(F) -> Emplace<F>;

ARC_MODULE_EXPORT
template<class F>
struct InPlace
{
    template<class Type>
    ARC_INLINE explicit(false) constexpr operator Type() &&
    {
        static_assert(std::is_same_v<Type, std::invoke_result_t<F, Constructor<Type>>>);
        return std::move(factory)(Constructor<Type>{});
    }

    [[no_unique_address]] F factory;
    // Ensure that InPlace is never relocated to avoid dangling references
    [[no_unique_address]] detail::NonRelocatable nonRelocatable{};
};

template<class F>
InPlace(F) -> InPlace<F>;

}

#endif // INCLUDE_ARC_FACTORY_HPP
