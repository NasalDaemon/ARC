#ifndef INCLUDE_ARC_NULL_TRAIT_HPP
#define INCLUDE_ARC_NULL_TRAIT_HPP

#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"
#include "arc/trait.hpp"
#include "arc/traits_fwd.hpp"

namespace arc {

ARC_MODULE_EXPORT
template<IsNodeHandle Node>
struct NoTrait : Trait
{
    template<class...>
    using Implements = void;
};

ARC_MODULE_EXPORT
template<IsNodeHandle Node>
inline constexpr NoTrait<Node> noTrait{};

ARC_MODULE_EXPORT
struct NullTrait : Trait
{
    template<class T>
    static void canProvide(NoTrait<T>);

    template<class...>
    using Implements = void;
};

namespace detail {
    template<class T>
    inline constexpr bool isNoTrait = false;

    template<IsNodeHandle Node>
    constexpr bool isNoTrait<NoTrait<Node>> = true;
}

ARC_MODULE_EXPORT
template<class T>
concept IsNoTrait = IsTrait<T> and detail::isNoTrait<T>;

ARC_MODULE_EXPORT
template<class T>
using NoTraits = arc::Traits<T, NullTrait>;

}


#endif // INCLUDE_ARC_NULL_TRAIT_HPP
