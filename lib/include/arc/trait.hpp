#ifndef INCLUDE_ARC_TRAIT_HPP
#define INCLUDE_ARC_TRAIT_HPP

#include "arc/detail/compress.hpp"
#include "arc/detail/concepts.hpp"
#include "arc/empty_types.hpp"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <type_traits>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<class... Ts>
struct AdlTag{};

ARC_MODULE_EXPORT
struct Trait
{
    struct Meta
    {
        struct Applicable{};
        struct Methods{};
        struct DuckMethods{};
    };

    template<class Self, std::same_as<Self> Expected>
    void canProvide(this Self, Expected);

    template<class Self>
    Self expects(this Self);
};

ARC_MODULE_EXPORT
struct UnconstrainedTrait : Trait
{
    template<class...>
    using Implements = void;
};

ARC_MODULE_EXPORT
template<class T>
concept IsTrait = requires (T trait) {
    requires std::is_base_of_v<Trait, T>;
    requires IsStateless<T>;
    requires std::is_default_constructible_v<T>;
    requires IsStateless<typename T::Meta>;
    requires IsStateless<typename T::Meta::Applicable>;
    requires IsStateless<typename T::Meta::Methods>;
    requires IsStateless<typename T::Meta::DuckMethods>;
    trait.expects();
    typename detail::TakesNaryClassTemplate<T::template Implements>;
};

ARC_MODULE_EXPORT
template<IsTrait Trait>
using TraitExpects = decltype(std::declval<Trait>().expects());

ARC_MODULE_EXPORT
template<class Trait, class Expected>
concept TraitCanProvide = requires (Trait trait, Expected expected) {
    requires IsTrait<Trait>;
    requires IsTrait<Expected>;
    trait.canProvide(expected);
};

ARC_MODULE_EXPORT
template<class T, class Trait>
concept MatchesTrait = TraitCanProvide<Trait, TraitExpects<T>>;

namespace detail {
    template<class Trait, class Impl, class Types, IsTrait... Subtraits>
    requires (sizeof...(Subtraits) > 0)
    using ImplementsAll = Void<typename Subtraits::template Implements<Trait, Decompress<Impl>, Decompress<Types>>...>;

    template<class Impl, class Types, IsTrait... Traits>
    requires (sizeof...(Traits) > 0)
    using Implements = Void<typename Traits::template Implements<Traits, Decompress<Impl>, Decompress<Types>>...>;
}

ARC_MODULE_EXPORT
template<class Impl, class Types, class... Traits>
concept Implements = requires { typename detail::Implements<Impl, Types, Traits...>; };

ARC_MODULE_EXPORT
template<class Method, class Trait>
concept IsMethodOf = IsTrait<Trait> and requires (Method method, Trait::Meta::Applicable meta) {
    meta.applicable(method);
};

ARC_MODULE_EXPORT
template<class Method>
using TraitOf = decltype(traitOf(std::declval<Method>()));

ARC_MODULE_EXPORT
template<IsTrait... Traits>
requires (sizeof...(Traits) > 0)
struct JoinedTrait : Traits...
{
    struct Meta
    {
        struct Applicable : Traits::Meta::Applicable...
        {
            using Traits::Meta::Applicable::applicable...;
        };
        struct Methods : Traits::Meta::Methods...
        {};
        struct DuckMethods : Traits::Meta::DuckMethods...
        {};
    };

    template<class Self, class Impl, class Types>
    using Implements = detail::ImplementsAll<Self, Impl, Types, Traits...>;

    template<class Expected>
    requires (... or TraitCanProvide<Traits, Expected>)
    static void canProvide(Expected);

    static void canProvide(JoinedTrait);

    static JoinedTrait expects();
};

ARC_MODULE_EXPORT
template<IsTrait Trait, class Id>
struct AltTrait : Trait
{
    static AltTrait expects();

    static void canProvide(AltTrait);
};

namespace detail {
    template<IsTrait Trait_>
    struct DuckTrait : arc::Trait
    {
        struct Meta
        {
            using Applicable = Trait_::Meta::Applicable;
            using Methods = Trait_::Meta::DuckMethods;
            using DuckMethods = Trait_::Meta::DuckMethods;
        };

        static TraitExpects<Trait_> expects();

        template<class Self, class...>
        using Implements = void;
    };
}

}


#endif // INCLUDE_ARC_TRAIT_HPP
