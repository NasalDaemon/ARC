#ifndef INCLUDE_ARC_KEY_HPP
#define INCLUDE_ARC_KEY_HPP

#include "arc/detail/concepts.hpp"

#include "arc/macros.hpp"
#include "arc/trait.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

namespace arc::key {

ARC_MODULE_EXPORT
template<class T>
concept IsKey = requires {
    typename arc::detail::TakesUnaryClassTemplate<T::template Trait>;
    typename arc::detail::TakesUnaryClassPackedAutoTemplate<T::template Interface>;
    requires std::is_trivially_copyable_v<T>;
};

ARC_MODULE_EXPORT
template<IsKey Key, IsTrait Trait_>
using Trait = Key::template Trait<Trait_>;

ARC_MODULE_EXPORT
struct Default
{
    template<class T>
    using Trait = T;
    template<class T, auto... Info>
    using Interface = T;

    template<class... Tags>
    static constexpr void assertCanAcquireAccess()
    {
        static_assert(sizeof...(Tags) == 0, "Access cannot be acquired with arc::key::Default");
    }

    template<class Source, class Target>
    static constexpr Target& acquireAccess(Source&, Target& target)
    {
        static_assert(arc::detail::alwaysFalse<Source, Target>, "Access denied");
        return target;
    }
};

}

#endif // INCLUDE_ARC_KEY_HPP
