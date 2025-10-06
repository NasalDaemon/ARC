#ifndef INCLUDE_ARC_EMPTY_TYPES_HPP
#define INCLUDE_ARC_EMPTY_TYPES_HPP

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

namespace arc {

ARC_MODULE_EXPORT
struct EmptyTypes{};

ARC_MODULE_EXPORT
template<class T>
concept IsStateless = std::is_empty_v<T>;

} // namespace arc

#endif // INCLUDE_ARC_EMPTY_TYPES_HPP
