#ifndef ARC_DETAIL_COMPRESS_HPP
#define ARC_DETAIL_COMPRESS_HPP

#if ARC_COMPRESS_TYPES

#include "arc/detail/cast.hpp"

#define ARC_COMPRESSOR_ID(id) _d1C##id##_

#define ARC_DEFINE_COMPRESSOR(id, Name) \
    consteval auto ARC_COMPRESSOR_ID(id)() \
    { \
        return []<class T>() \
        { \
            struct Name : T \
            { \
                static T _arc_uncompressedType_(); \
                static Name _arc_compressedType_(); \
            }; \
            return std::type_identity<Name>{}; \
        }; \
    }

ARC_DEFINE_COMPRESSOR(c, Context)
ARC_DEFINE_COMPRESSOR(i, Impl)
ARC_DEFINE_COMPRESSOR(t, Types)

#undef ARC_DEFINE_COMPRESSOR

#define ARC_COMPRESSOR(id, T) \
    ::ARC_COMPRESSOR_ID(id)().template operator()<std::remove_cvref_t<T>>()

namespace arc::detail {

    template<class T>
    concept IsCompressed = requires(T t) {
        t._arc_uncompressedType_();
        { t._arc_compressedType_() } -> std::same_as<std::remove_cvref_t<T>>;
    };

    template<class T>
    T decompressed();
    template<IsCompressed T>
    auto decompressed() -> decltype(T::_arc_uncompressedType_());

    template<class T>
    using Decompress = decltype(decompressed<std::remove_cvref_t<T>>());

    template<class T>
    requires (not IsCompressed<T>)
    using CompressContext = decltype(ARC_COMPRESSOR(c, T))::type;

    template<class T>
    requires (not IsCompressed<T>)
    using CompressImpl = decltype(ARC_COMPRESSOR(i, T))::type;

    template<class T>
    requires (not IsCompressed<T>)
    using CompressTypes = decltype(ARC_COMPRESSOR(t, T))::type;

    template<class T>
    requires (not IsCompressed<T>)
    constexpr auto& compressImpl(T& impl)
    {
        if constexpr (not std::is_final_v<T>)
            return downCast<CompressImpl<T>>(impl);
        else
            return impl;
    }

} // namespace arc::detail

#undef ARC_COMPRESSOR_ID
#undef ARC_COMPRESSOR

#else

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

namespace arc::detail {

    template<class T>
    concept IsCompressed = false;

    template<class T>
    using Decompress = std::remove_cvref_t<T>;

    template<class T>
    using CompressContext = std::remove_cvref_t<T>;

    template<class T>
    using CompressImpl = std::remove_cvref_t<T>;

    template<class T>
    using CompressTypes = std::remove_cvref_t<T>;

    template<class T>
    constexpr T& compressImpl(T& impl)
    {
        return impl;
    }

} // namespace arc::detail

#endif

#endif // ARC_DETAIL_COMPRESS_HPP
