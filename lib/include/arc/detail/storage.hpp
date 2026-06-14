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

template<class T>
struct StorageBuffer
{
    constexpr StorageBuffer(std::size_t capacity)
        : data(capacity ? new Storage<T>[capacity] : nullptr)
    {}

    StorageBuffer() = default;

    constexpr T* storageAt(std::size_t index) noexcept { return static_cast<T*>(data.get()) + index; }
    constexpr const T* storageAt(std::size_t index) const noexcept { return static_cast<const T*>(data.get()) + index; }
    constexpr T* valueAt(std::size_t index) noexcept { return std::launder(storageAt(index)); }
    constexpr const T* valueAt(std::size_t index) const noexcept { return std::launder(storageAt(index)); }

    void reset() noexcept { data.reset(); }

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

} // namespace arc::detail

#endif // INCLUDE_ARC_DETAIL_STORAGE_HPP
