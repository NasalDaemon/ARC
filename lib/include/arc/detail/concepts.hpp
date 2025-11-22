#ifndef INCLUDE_ARC_DETAIL_CONCEPTS_HPP
#define INCLUDE_ARC_DETAIL_CONCEPTS_HPP

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <type_traits>
#endif

namespace arc::detail {

// Concept to check if T is constructible from an rvalue of type T.
// Handles the case where T's move constructor is deleted but copy constructor is available
// std::move_constructible fails in such cases
template<class T>
concept MoveConstructible = std::constructible_from<T, T&&>;

template<class T>
concept CopyConstructible = std::constructible_from<T, T const&>;

template<template<class> class Template>
using TakesUnaryClassTemplate = void;

template<template<class...> class Template>
using TakesNaryClassTemplate = void;

template<template<class, auto...> class Template>
using TakesUnaryClassPackedAutoTemplate = void;

template<class...>
inline constexpr bool alwaysTrue = true;
template<class...>
inline constexpr bool alwaysFalse = false;

template<class...>
using Void = void;

template<class T>
requires std::is_void_v<T>
using VoidOnly = void;

template<class... Ts>
using AllVoid = Void<VoidOnly<Ts>...>;

} // namespace arc::detail


#endif // INCLUDE_ARC_DETAIL_CONCEPTS_HPP
