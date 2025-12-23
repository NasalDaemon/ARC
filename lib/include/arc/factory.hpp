#ifndef INCLUDE_ARC_FACTORY_HPP
#define INCLUDE_ARC_FACTORY_HPP

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <utility>
#endif

namespace arc {

ARC_MODULE_EXPORT
struct WithFactory {} inline constexpr withFactory{};

ARC_MODULE_EXPORT
template<class Type>
struct Constructor
{
    using type = Type;

    template<class... Args>
    requires std::constructible_from<Type, Args...>
    constexpr Type operator()(Args&&... args) const
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
    explicit constexpr operator Type() &&
    {
        std::same_as<Type> decltype(auto) result = std::move(factory)(Constructor<Type>{});
        return result;
    }

    [[no_unique_address]] F factory;
    // Ensure that Emplace is never relocated to avoid dangling references
    [[no_unique_address]] detail::NonRelocatable nonRelocatable{};
};

template<class F>
Emplace(F) -> Emplace<F>;

}

#endif // INCLUDE_ARC_FACTORY_HPP
