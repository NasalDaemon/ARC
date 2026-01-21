#ifndef INCLUDE_ARC_BITSET_HPP
#define INCLUDE_ARC_BITSET_HPP

#include "arc/macros.hpp"

namespace arc {

namespace detail {
    template<std::size_t N>
    consteval auto bitsetFor()
    {
        if constexpr (N <= 8)
            return std::uint8_t(0);
        else if constexpr (N <= 16)
            return std::uint16_t(0);
        else if constexpr (N <= 32)
            return std::uint32_t(0);
        else if constexpr (N <= 64)
            return std::uint64_t(0);
        else
            throw "Bitset size too large";
    }
}

ARC_MODULE_EXPORT
template<std::size_t Bits>
requires (Bits <= 64)
struct Bitset
{
    Bitset() = default;

    ARC_INLINE constexpr void set(auto index)
    {
        bits |= One << index;
    }

    ARC_INLINE constexpr void unset(auto index)
    {
        bits &= ~(One << index);
    }

    ARC_INLINE constexpr bool test(auto index) const
    {
        return (bits & (One << index)) != 0;
    }

    ARC_INLINE constexpr void reset()
    {
        bits = 0;
    }

    ARC_INLINE constexpr bool any() const
    {
        return bits != 0;
    }

    ARC_INLINE constexpr bool none() const
    {
        return bits == 0;
    }

private:
    using StorageType = decltype(detail::bitsetFor<Bits>());
    StorageType bits = 0;
    static constexpr StorageType One = 1;
};

}

#endif // INCLUDE_ARC_BITSET_HPP
