#ifndef INCLUDE_ARC_GLOBAL_TRAIT_HPP
#define INCLUDE_ARC_GLOBAL_TRAIT_HPP

#include "arc/macros.hpp"
#include "arc/trait.hpp"

namespace arc {

ARC_MODULE_EXPORT
template<IsTrait T>
struct Global;

namespace detail {
    template<class T>
    inline constexpr bool isGlobalTrait = false;

    template<class T>
    constexpr bool isGlobalTrait<Global<T>> = true;

    template<class T>
    concept IsGlobalTrait = isGlobalTrait<T>;
}

ARC_MODULE_EXPORT
template<class T>
concept IsGlobalTrait = IsTrait<T> and detail::IsGlobalTrait<T>;

ARC_MODULE_EXPORT
template<class T>
concept IsNonGlobalTrait = IsTrait<T> and not detail::IsGlobalTrait<T>;

ARC_MODULE_EXPORT
template<IsTrait T>
struct Global : T
{
    static_assert(IsNonGlobalTrait<T>, "Global trait must wrap a non-global trait");

    using Trait = T;

    struct Meta : T::Meta
    {
        using Resolver = T::Meta::GlobalResolver;
    };

    static Global expects();
    static void canProvide(Trait);
};

ARC_MODULE_EXPORT
template<IsGlobalTrait Trait>
constexpr Trait global(Trait = {}) { return {}; }

ARC_MODULE_EXPORT
template<IsTrait Trait>
constexpr Global<Trait> global(Trait = {}) { return {}; }

} // namespace arc


#endif // INCLUDE_ARC_GLOBAL_TRAIT_HPP
