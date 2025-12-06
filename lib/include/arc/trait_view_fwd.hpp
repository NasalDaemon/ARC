#ifndef INCLUDE_ARC_TRAIT_VIEW_FWD_HPP
#define INCLUDE_ARC_TRAIT_VIEW_FWD_HPP

#include "arc/empty_types.hpp"
#include "arc/key.hpp"
#include "arc/macros.hpp"
#include "arc/trait.hpp"

#if !ARC_IMPORT_STD
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
template<IsTrait Trait>
struct AutoCompleteTraitView;

template<IsTrait Trait, class ImplAlias>
struct TraitNodeView;

// Preserve constness of ImplAlias when constructing from TraitView
template<IsTrait Trait, class ImplAlias>
TraitNodeView(Trait, ImplAlias&) -> TraitNodeView<Trait, ImplAlias>;

// Presents a view over a trait implementation, where only the trait trait functions are accessible
ARC_MODULE_EXPORT
template<IsTrait Trait, class ImplAlias, class Types_ = EmptyTypes>
struct TraitView;

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
constexpr IsTraitViewOf<Trait, Key> auto makeTraitView(auto& source, auto target, Trait, Key const& key, Keys const&... keys);

} // namespace arc


#endif // INCLUDE_ARC_TRAIT_VIEW_HPP
