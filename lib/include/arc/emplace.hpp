#ifndef INCLUDE_ARC_FACTORY_HPP
#define INCLUDE_ARC_FACTORY_HPP

#include "arc/emplace_fwd.hpp"

#include "arc/macros.hpp"
#include "arc/static_assert.hpp"

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
        ARC_STATIC_ASSERT_INLINE(std::is_same_v<Type, std::invoke_result_t<F, Constructor<Type>>>)(
            "arc::Emplace factory must return exactly the target type",
            (target, arc::typeName<Type>)(returned, arc::typeName<std::invoke_result_t<F, Constructor<Type>>>));
        return std::move(factory)(Constructor<Type>{});
    }

    ARC_NO_UNIQUE_ADDRESS F factory;
    // Ensure that Emplace is never relocated to avoid dangling references
    ARC_NO_UNIQUE_ADDRESS detail::NonRelocatable nonRelocatable{};
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
        ARC_STATIC_ASSERT_INLINE(std::is_same_v<Type, std::invoke_result_t<F, Constructor<Type>>>)(
            "arc::InPlace factory must return exactly the target type",
            (target, arc::typeName<Type>)(returned, arc::typeName<std::invoke_result_t<F, Constructor<Type>>>));
        return std::move(factory)(Constructor<Type>{});
    }

    ARC_NO_UNIQUE_ADDRESS F factory;
    // Ensure that InPlace is never relocated to avoid dangling references
    ARC_NO_UNIQUE_ADDRESS detail::NonRelocatable nonRelocatable{};
};

template<class F>
InPlace(F) -> InPlace<F>;

}

#endif // INCLUDE_ARC_FACTORY_HPP
