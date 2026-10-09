#ifndef INCLUDE_ARC_EMPLACE_FWD_HPP
#define INCLUDE_ARC_EMPLACE_FWD_HPP

// Declarations of the types defined in arc/emplace.hpp, for headers that arc/emplace.hpp itself depends on

#include "arc/macros.hpp"

namespace arc {

ARC_MODULE_EXPORT
template<class Type>
struct Constructor;

ARC_MODULE_EXPORT
template<class F>
struct Emplace;

ARC_MODULE_EXPORT
template<class F>
struct InPlace;

} // namespace arc

#endif // INCLUDE_ARC_EMPLACE_FWD_HPP
