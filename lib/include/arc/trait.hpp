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
struct DisableNamedImpl{};

ARC_MODULE_EXPORT
template<class T>
struct DisableNamedImplFor : DisableNamedImpl {};

ARC_MODULE_EXPORT
struct Trait
{
    struct Meta
    {
        struct Applicable{};
        struct Methods{};
        struct DuckMethods{};
        struct NamedMethods{};
        struct Resolver{};
        struct GlobalResolver{};
        struct Converter{};
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
    requires IsStateless<typename T::Meta::NamedMethods>;
    requires IsStateless<typename T::Meta::DuckMethods>;
    requires IsStateless<typename T::Meta::Resolver>;
    requires IsStateless<typename T::Meta::GlobalResolver>;
    requires IsStateless<typename T::Meta::Converter>;
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
    using ImplementsAll = ToVoid<typename Subtraits::template Implements<Trait, Decompress<Impl>, Decompress<Types>>...>;

    template<class Impl, class Types, IsTrait... Traits>
    requires (sizeof...(Traits) > 0)
    using Implements = ToVoid<typename Traits::template Implements<Traits, Decompress<Impl>, Decompress<Types>>...>;
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
        struct NamedMethods : Traits::Meta::NamedMethods...
        {
            using Traits::Meta::NamedMethods::impl...;
        };
        struct DuckMethods : Traits::Meta::DuckMethods...
        {};
        struct Resolver : Traits::Meta::Resolver...
        {};
        struct GlobalResolver : Traits::Meta::GlobalResolver...
        {};
        struct Converter : Traits::Meta::Converter...
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
template<IsTrait Trait_, class Id>
struct AltTrait : Trait_
{
    struct Meta : Trait_::Meta
    {
        // Erase resolvers, as the name has changed
        struct Resolver{};
        struct GlobalResolver{};
        struct Converter{};
        // Named methods should lose the original name
        struct NamedMethods : Trait_::Meta::Methods
        {
            static void impl() = delete;
        };
    };

    static AltTrait expects();

    static void canProvide(AltTrait);
};

namespace detail {
    template<IsTrait Trait_>
    struct DuckTrait : arc::Trait
    {
        struct Meta : Trait_::Meta
        {
            // All methods are made duck methods
            using Methods = Trait_::Meta::DuckMethods;
        };

        static TraitExpects<Trait_> expects();

        template<class Self, class...>
        using Implements = void;
    };
}

}


#endif // INCLUDE_ARC_TRAIT_HPP
