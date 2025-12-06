#ifndef ARC_DETAIL_TYPE_NAME_HPP
#define ARC_DETAIL_TYPE_NAME_HPP

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <cstddef>
#include <source_location>
#include <string_view>
#include <type_traits>
#endif

namespace arc {

namespace detail {

    using TypeNameProber = void;

    template<class T>
    consteval std::string_view wrappedTypeName()
    {
        return std::source_location::current().function_name();
    }

    consteval std::size_t wrappedTypeNamePrefixLength();
    consteval std::size_t wrappedTypeNameSuffixLength();

    template<class T>
    consteval std::string_view typeName()
    {
        if constexpr (std::is_void_v<T>)
        {
            return "void";
        }
        else
        {
            constexpr auto wrappedName = wrappedTypeName<T>();
            constexpr auto typeNameLength = wrappedName.length() - wrappedTypeNamePrefixLength() - wrappedTypeNameSuffixLength();
            return wrappedName.substr(wrappedTypeNamePrefixLength(), typeNameLength);
        }
    }

    consteval std::size_t wrappedTypeNamePrefixLength()
    {
        return wrappedTypeName<TypeNameProber>().find(typeName<TypeNameProber>());
    }

    consteval std::size_t wrappedTypeNameSuffixLength()
    {
        return wrappedTypeName<TypeNameProber>().length()
            - wrappedTypeNamePrefixLength()
            - typeName<TypeNameProber>().length();
    }

} // namespace detail

ARC_MODULE_EXPORT
template<class T>
inline constexpr std::string_view typeName = detail::typeName<T>();

} // namespace arc

#endif // ARC_DETAIL_TYPE_NAME_HPP
