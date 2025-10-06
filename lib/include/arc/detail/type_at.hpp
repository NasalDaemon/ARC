#ifndef INCLUDE_ARC_DETAIL_TYPE_AT_HPP
#define INCLUDE_ARC_DETAIL_TYPE_AT_HPP

#include "arc/macros.hpp"

#if ARC_CPP_VERSION > 202302L and __cpp_pack_indexing >= 202311L
#   define ARC_TYPE_AT_VER 1
#elif ARC_COMPILER_GNU
#   define ARC_TYPE_AT_VER 2
#endif

#if !ARC_IMPORT_STD
#include <cstdint>
#   if !ARC_TYPE_AT_VER
#   include <type_traits>
#   include <utility>
#   endif
#endif

namespace arc::detail {

#if ARC_TYPE_AT_VER == 1

template<std::size_t I, class... Ts>
using TypeAt = Ts...[I];

#elif ARC_TYPE_AT_VER == 2

template<std::size_t I, class... Ts>
requires (I < sizeof...(Ts))
using TypeAt = __type_pack_element<I, Ts...>;

#else

struct Sink
{
    consteval Sink(auto) {}
};

template<class... Sinks, class T>
T getTypeAt2(Sinks..., std::type_identity<T>, auto...);

template<class... Ts, std::size_t... Is>
auto getTypeAt1(std::index_sequence<Is...>) -> decltype(getTypeAt2<decltype(Sink(Is))...>(std::type_identity<Ts>()...));

template<std::size_t I, class... Ts>
requires (I < sizeof...(Ts))
using TypeAt = decltype(getTypeAt1<Ts...>(std::make_index_sequence<I>{}));

#endif

}


#endif // INCLUDE_ARC_DETAIL_TYPE_AT_HPP
