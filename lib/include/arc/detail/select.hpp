#ifndef INCLUDE_ARC_DETAIL_ENABLE_IF_HPP
#define INCLUDE_ARC_DETAIL_ENABLE_IF_HPP

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

namespace arc::detail {

template<bool B>
struct EnableID
{
    template<class T>
    using ID = std::type_identity<T>;
};

template<>
struct EnableID<false>
{
    struct Disabled
    {};

    template<class T>
    using ID = Disabled;
};

template<bool B, class T>
using EnableIf = EnableID<B>::template ID<T>;

template<class... Bases>
struct InheritAll : Bases...
{};

template<class Pred, class... Ts>
using SelectIf = InheritAll<EnableIf<Pred::template value<Ts>, Ts>...>::type;

struct Empty{};

template<bool B, class T>
using EmptyIf = std::conditional_t<B, Empty, T>;

} // namespace arc::detail


#endif // INCLUDE_ARC_DETAIL_ENABLE_IF_HPP
