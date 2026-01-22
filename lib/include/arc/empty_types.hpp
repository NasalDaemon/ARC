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

namespace detail {
    struct Void{};

    template<class T>
    auto normaliseVoid() -> T;
    template<IsStateless T>
    auto normaliseVoid() -> Void;

    template<class T>
    using NormaliseVoid = decltype(normaliseVoid<T>());
}

} // namespace arc

#endif // INCLUDE_ARC_EMPTY_TYPES_HPP
