#ifndef ARC_TYPE_ID_HPP
#define ARC_TYPE_ID_HPP

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <bit>
#include <cstddef>
#include <functional>
#endif

namespace arc {

namespace detail {
    template<class... Ts>
    char const uniqueId{};
} // namespace detail

ARC_MODULE_EXPORT
struct TypeId
{
    template<class... Ts>
    static TypeId of()
    {
        return TypeId(&detail::uniqueId<Ts...>);
    }

    auto operator<=>(TypeId const&) const = default;

private:
    constexpr explicit TypeId(char const* id)
        : id(id)
    {}

    friend struct std::hash<TypeId>;
    char const* id;
};

} // namespace arc::detail

template<>
struct std::hash<arc::TypeId>
{
    constexpr std::size_t operator()(arc::TypeId const& typeId) const noexcept
    {
        std::size_t hash = std::bit_cast<std::size_t>(typeId.id);

        // MurmurHash3 finalizer - excellent avalanche properties
        // hash ^= std::rotr(hash, 33);
        // hash *= 0xff51afd7ed558ccdul;
        // hash ^= std::rotr(hash, 33);
        return hash;
    }
};


#endif // ARC_TYPE_ID_HPP
