#ifndef INCLUDE_ARC_DETAIL_ENABLE_IF_HPP
#define INCLUDE_ARC_DETAIL_ENABLE_IF_HPP

#include "arc/macros.hpp"
#include "arc/detail/type_at.hpp"

#if !ARC_IMPORT_STD
#include <cstddef>
#include <type_traits>
#endif

namespace arc::detail {

// Position of the first T in Ts... satisfying Pred<T, _>, or sizeof...(Ts) when
// none does. Selecting by index is what keeps SelectIf cheap: the alias below
// then resolves through TypeAt, which is a compiler builtin (pack indexing /
// __type_pack_element) and instantiates nothing. The only entities left behind
// are the Pred specialisations themselves, which the compiler memoises across
// queries.
//
// Two or more matches is a wiring bug (e.g. two cluster members implementing
// one trait), and the throw reports it: in a consteval function it makes the
// call a non-constant expression, so the diagnostic points here.
template<template<class, class> class Pred, class T, class... Ts>
consteval std::size_t selectIndex()
{
    // Trailing sentinel: the array is never zero-sized, even for an empty pack.
    bool const matches[]{Pred<T, Ts>::value..., true};
    std::size_t first = sizeof...(Ts);
    std::size_t count = 0;
    for (std::size_t i = 0; i < sizeof...(Ts); ++i)
        if (matches[i])
            if (count++ == 0)
                first = i;

    if (count > 1)
        throw "SelectIf: more than one candidate satisfies the predicate";
    return first;
}

// The one T in Ts... satisfying Pred<T, _>.
//
// Constrained rather than ill-formed when nothing matches, so a caller probing
// with a requires-clause gets a substitution failure instead of a hard error.
// Callers constrain on "at least one candidate matches" anyway, which is where
// the readable diagnostic comes from.
template<template<class, class> class Pred, class T, class... Ts>
requires (selectIndex<Pred, T, Ts...>() < sizeof...(Ts))
using SelectIf = TypeAt<selectIndex<Pred, T, Ts...>(), Ts...>;

struct Empty{};

template<bool B, class T>
using EmptyIf = std::conditional_t<B, Empty, T>;

} // namespace arc::detail


#endif // INCLUDE_ARC_DETAIL_ENABLE_IF_HPP
