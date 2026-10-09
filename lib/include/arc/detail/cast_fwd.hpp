#ifndef INCLUDE_ARC_DETAIL_CAST_FWD_HPP
#define INCLUDE_ARC_DETAIL_CAST_FWD_HPP

// Declarations of the casts defined in arc/detail/cast.hpp, for headers that arc/detail/cast.hpp itself depends on

#include "arc/macros.hpp"

namespace arc::detail {

template<class Derived, class Base>
ARC_INLINE constexpr Derived& downCast(Base& base);

template<class Derived, class Base>
ARC_INLINE constexpr Derived const& downCast(Base const& base);

template<class Base, class Derived>
ARC_INLINE constexpr Base& upCast(Derived& derived);

template<class Base, class Derived>
ARC_INLINE constexpr Base const& upCast(Derived const& derived);

} // namespace arc::detail

#endif // INCLUDE_ARC_DETAIL_CAST_FWD_HPP
