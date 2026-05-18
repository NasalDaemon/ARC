#ifndef INCLUDE_ARC_DETAIL_STORAGE_HPP
#define INCLUDE_ARC_DETAIL_STORAGE_HPP

#include "arc/empty_types.hpp"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <cstddef>
#include <memory>
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

template<IsStateless T>
struct Storage<T>
{
    constexpr Storage() {} // explicit no-op constructor to prevent zero-initialization

    constexpr T* storage() { return std::addressof(t); }
    constexpr const T* storage() const { return std::addressof(t); }
    constexpr T* value() { return storage(); }
    constexpr const T* value() const { return storage(); }

private:
    [[no_unique_address]] T t ARC_INDETERMINATE;
};

}

#endif // INCLUDE_ARC_DETAIL_STORAGE_HPP
