#ifndef INCLUDE_ARC_DETAIL_STORAGE_HPP
#define INCLUDE_ARC_DETAIL_STORAGE_HPP

#include "arc/empty_types.hpp"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <cstddef>
#include <memory>
#include <type_traits>
#endif

namespace arc::detail {

template<class T>
union StorageUnion
{
    constexpr StorageUnion() {}
    constexpr ~StorageUnion() requires std::is_trivially_destructible_v<T> = default;
    constexpr ~StorageUnion() {}
    T t;
};

template<class T>
struct Storage
{
    constexpr Storage() {} // explicit no-op constructor to leave t uninitialised

    constexpr T* value() { return std::addressof(u.t); }
    constexpr const T* value() const { return std::addressof(u.t); }

private:
    // u is not [[no_unique_address]]: a potentially-overlapping subobject is not
    // transparently replaceable, so u.t would no longer name the object constructed in it
    StorageUnion<T> u;
};

template<IsStateless T>
struct Storage<T>
{
    constexpr Storage() {} // explicit no-op constructor to prevent zero-initialization

    constexpr T* value() { return std::addressof(t); }
    constexpr const T* value() const { return std::addressof(t); }

private:
    ARC_NO_UNIQUE_ADDRESS T t;
};

// GCC < 16 crashes exporting a constexpr function containing an array new-expression
// of a type with a non-trivial destructor from a module
#if ARC_COMPILER_LT(GCC, 16)
#   define ARC_STORAGE_BUFFER_CONSTEXPR
#else
#   define ARC_STORAGE_BUFFER_CONSTEXPR constexpr
#endif

template<class T>
struct StorageBuffer
{
    ARC_STORAGE_BUFFER_CONSTEXPR StorageBuffer(std::size_t capacity)
        : data(capacity ? new Storage<T>[capacity] : nullptr)
    {}

    StorageBuffer() = default;

    constexpr T* valueAt(std::size_t index) noexcept { return static_cast<Storage<T>*>(data.get())[index].value(); }
    constexpr const T* valueAt(std::size_t index) const noexcept { return static_cast<Storage<T> const*>(data.get())[index].value(); }

    constexpr void reset() noexcept { data.reset(); }

private:
    struct Deleter
    {
        constexpr void operator()(void* p) const noexcept
        {
            delete[] static_cast<Storage<T>*>(p);
        }
    };

    std::unique_ptr<void, Deleter> data;
};

#undef ARC_STORAGE_BUFFER_CONSTEXPR

} // namespace arc::detail

#endif // INCLUDE_ARC_DETAIL_STORAGE_HPP
