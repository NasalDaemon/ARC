#ifndef INCLUDE_ARC_DETAIL_STORAGE_HPP
#define INCLUDE_ARC_DETAIL_STORAGE_HPP

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <cstddef>
#include <new>
#endif

namespace arc::detail {

template<class T>
struct Storage
{
    constexpr Storage() {} // explicit no-op constructor to prevent zero-initialization of bytes

    constexpr T* storage() { return reinterpret_cast<T*>(bytes); }
    constexpr const T* storage() const { return reinterpret_cast<const T*>(bytes); }
    constexpr T* value() { return std::launder(storage()); }
    constexpr const T* value() const { return std::launder(storage()); }

private:
    alignas(T) std::byte bytes[sizeof(T)] ARC_INDETERMINATE;
};

}

#endif // INCLUDE_ARC_DETAIL_STORAGE_HPP
