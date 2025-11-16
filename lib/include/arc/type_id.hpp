#ifndef ARC_TYPE_ID_HPP
#define ARC_TYPE_ID_HPP

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <bit>
#include <cstddef>
#include <functional>
#endif

namespace arc {

ARC_MODULE_EXPORT
struct TypeId
{
    template<class... Ts>
    static TypeId of()
    {
        return TypeId(std::bit_cast<std::size_t>(&fn<Ts...>));
    }

    auto operator<=>(TypeId const&) const = default;

private:
    constexpr explicit TypeId(std::size_t id)
        : id(id)
    {}

    template<class... Ts>
    static void fn() {}

    friend struct std::hash<TypeId>;
    std::size_t id;
};

} // namespace arc::detail

template<>
struct std::hash<arc::TypeId>
{
    constexpr std::size_t operator()(arc::TypeId const& typeId) const noexcept
    {
        // Function pointers are at least 16 byte aligned, so the lower 4 bits contain no entropy.
        // Rotate right to concentrate entropy towards the lower bits for better modulo-based bucketing.
        std::size_t hash = std::rotr(typeId.id, 4);

        // MurmurHash3 finalizer - excellent avalanche properties
        hash ^= std::rotr(hash, 33);
        hash *= 0xff51afd7ed558ccdul;
        hash ^= std::rotr(hash, 33);
        return hash;
    }
};


#endif // ARC_TYPE_ID_HPP
