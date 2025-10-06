#ifndef INCLUDE_ARC_TRAIT_VIEW_HPP
#define INCLUDE_ARC_TRAIT_VIEW_HPP

#include "arc/detail/concepts.hpp"

#include "arc/alias.hpp"
#include "arc/empty_types.hpp"
#include "arc/key.hpp"
#include "arc/macros.hpp"
#include "arc/no_trait.hpp"
#include "arc/trait.hpp"
#include "arc/traits_fwd.hpp"

#if !ARC_IMPORT_STD
#include <functional>
#include <type_traits>
#endif

namespace arc {

namespace detail {
    template<class T>
    inline constexpr bool isTraitView = false;
}

ARC_MODULE_EXPORT
template<class T>
concept IsTraitView = detail::isTraitView<std::remove_cvref_t<T>>;

ARC_MODULE_EXPORT
template<class T, class Trait, class Key = key::Default>
concept IsTraitViewOf = IsTraitView<T> and IsTrait<Trait> and std::derived_from<T, typename key::Trait<Key, Trait>::Meta::Methods>;

ARC_MODULE_EXPORT
struct AsFunctor{} inline constexpr asFunctor;

ARC_MODULE_EXPORT
template<class Trait, IsMethodOf<Trait> Method, class AliasImpl>
struct TraitMethodFunctor
{
    explicit constexpr TraitMethodFunctor(AliasImpl impl) : impl(impl) {}

    ARC_INLINE constexpr decltype(auto) operator()(this auto&& self, auto&&... args)
    {
        return self.impl->impl(Method{}, ARC_FWD(args)...);
    }

private:
    AliasImpl impl;
};

namespace detail {
    struct Unknown
    {
        template<class T>
        explicit(false) constexpr operator T&&();
        template<class T>
        explicit(false) constexpr operator T&();
    };
}

struct AutoCompleteTraitNodeView
{
    ARC_INLINE constexpr auto* operator->(this auto&& self) { return std::addressof(self); }

    constexpr detail::Unknown visit(this auto&& self, auto&& visitor);
    constexpr detail::Unknown getElementId() const;
};

ARC_MODULE_EXPORT
template<IsTrait Trait>
struct AutoCompleteTraitView final : Trait::Meta::Methods
{
    static consteval std::true_type isTrait(MatchesTrait<Trait> auto) { return {}; }
    static consteval std::false_type isTrait(auto) { return {}; }

    using Types = EmptyTypes;
    struct Traits
    {
        template<IsTrait T>
        using ResolveInterface = AutoCompleteTraitView;

        template<IsTrait T>
        using ResolveTypes = Types;
    };

    template<IsMethodOf<Trait> Method>
    constexpr detail::Unknown impl(this auto&&, Method, auto&&...);

    constexpr AutoCompleteTraitNodeView operator->() const { return {}; }
};

template<IsTrait Trait, class ImplAlias>
struct TraitNodeView final
{
    constexpr explicit TraitNodeView(Trait, ImplAlias alias) : alias(alias) {}

    ARC_INLINE constexpr auto* operator->(this auto&& self) { return std::addressof(self); }

    // Visit the TraitView of the concrete implementation (e.g. active node of union)
    template<class Visitor>
    constexpr decltype(auto) visit(this auto&& self, Visitor&& visitor)
    {
        return self.alias->visit(
            [&](auto& impl) -> decltype(auto)
            {
                return std::invoke(ARC_FWD(Visitor, visitor), impl.asTrait(Trait{}));
            });
    }

    ARC_INLINE constexpr auto const& getElementId() const
    {
        return alias->getElementId();
    }

    ARC_INLINE constexpr auto getElementHandle() const
    {
        return alias->getElementHandle();
    }

private:
    ImplAlias alias;
};

// Preserve constness of ImplAlias when constructing from TraitView
template<IsTrait Trait, class ImplAlias>
TraitNodeView(Trait, ImplAlias&) -> TraitNodeView<Trait, ImplAlias>;

// Presents a view over a trait implementation, where only the trait trait functions are accessible
ARC_MODULE_EXPORT
template<IsTrait Trait, class ImplAlias, class Types_ = EmptyTypes>
struct TraitView final : Trait::Meta::Methods
{
    constexpr TraitView(Trait, ImplAlias alias, std::type_identity<Types_>)
        : alias(alias)
    {
        static_assert(not IsTrait<TraitView>);
        ARC_ASSERT_IMPLEMENTS(typename ImplAlias::Interface, Types, Trait);
        if constexpr (not std::is_const_v<typename ImplAlias::Impl>)
            ARC_ASSERT_IMPLEMENTS(TraitView, Types, Trait);
    }

    static consteval std::true_type isTrait(MatchesTrait<Trait> auto) { return {}; }
    static consteval std::false_type isTrait(auto) { return {}; }

    using Types = detail::Decompress<Types_>;
    struct Traits
    {
        using Node = ImplAlias::Impl;

        template<std::same_as<TraitView> = TraitView>
        using GetContext = ContextOf<typename ImplAlias::Impl>;

        template<std::same_as<Trait> T>
        using ResolveInterface = TraitView;

        template<std::same_as<Trait> T>
        using ResolveTypes = Types;
    };

    template<IsMethodOf<Trait> Method>
    ARC_INLINE constexpr decltype(auto) impl(this auto&& self, Method trait, auto&&... args)
    {
        return self.alias->impl(trait, ARC_FWD(args)...);
    }

    template<IsMethodOf<Trait> Method>
    ARC_INLINE constexpr TraitMethodFunctor<Trait, Method, ImplAlias> impl(this auto&& self, Method, arc::AsFunctor)
    {
        return TraitMethodFunctor<Trait, Method, ImplAlias>(self.alias);
    }

    ARC_INLINE auto operator->(this auto&& self)
    {
        return TraitNodeView(Trait{}, self.alias);
    }

private:
    ImplAlias alias;
};

template<IsNoTrait Trait, class ImplAlias, class Types_>
struct TraitView<Trait, ImplAlias, Types_> final : Trait::Meta::Methods
{
    constexpr TraitView(Trait, ImplAlias alias, std::type_identity<Types_>)
        : alias(alias)
    {}

    static consteval std::false_type isTrait(auto) { return {}; }

    using Types = detail::Decompress<Types_>;
    struct Traits
    {
        using Node = ImplAlias::Impl;

        template<std::same_as<TraitView> = TraitView>
        using GetContext = ContextOf<typename ImplAlias::Impl>;

        template<std::same_as<Trait> T>
        using ResolveInterface = TraitView;

        template<std::same_as<Trait> T>
        using ResolveTypes = Types;
    };

    ARC_INLINE auto operator->(this auto&& self)
    {
        return self.alias;
    }

private:
    ImplAlias alias;
};

ARC_MODULE_EXPORT
template<IsTrait Trait, class ImplAlias, class Types>
TraitView(Trait, ImplAlias, std::type_identity<Types>) -> TraitView<Trait, ImplAlias, Types>;

namespace detail {
    template<class Trait, class Impl, class Types>
    constexpr bool isTraitView<TraitView<Trait, Impl, Types>> = true;
    template<class Trait>
    constexpr bool isTraitView<AutoCompleteTraitView<Trait>> = true;
}

ARC_MODULE_EXPORT
template<IsTrait Trait, key::IsKey Key, key::IsKey... Keys>
constexpr IsTraitViewOf<Trait, Key> auto makeTraitView(auto& source, auto target, Trait, Key const& key, Keys const&... keys)
{
    return TraitView(key::Trait<Key, Trait>{}, target.ptr->finalise(source, key, keys...), target.types());
}


} // namespace arc


#endif // INCLUDE_ARC_TRAIT_VIEW_HPP
