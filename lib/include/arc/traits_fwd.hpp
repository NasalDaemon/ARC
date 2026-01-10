#ifndef INCLUDE_ARC_TRAITS_FWD_HPP
#define INCLUDE_ARC_TRAITS_FWD_HPP

#include "arc/context_fwd.hpp"
#include "arc/empty_types.hpp"
#include "arc/link.hpp"
#include "arc/macros.hpp"
#include "arc/trait.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<class Interface_, class Types_>
struct ResolvedTrait
{
    using Interface = Interface_;
    using Types = Types_;
};

ARC_MODULE_EXPORT
template<class T>
concept IsResolvedTrait = requires
{
    typename T::Interface;
    typename T::Types;
};

namespace detail {

    template<class Node>
    concept HasNodeTraits = requires {
        typename Node::Traits;
    };

    template<class Trait_>
    struct TraitsItem
    {
        using Trait = Trait_;
    };
    template<class Trait_, class I>
    struct TraitsItem<Trait_(I)>
    {
        using Trait = Trait_;
    };
    template<class Trait_, class I, class T>
    struct TraitsItem<Trait_(I, T)>
    {
        using Trait = Trait_;
    };
    template<class Trait_, class T>
    struct TraitsItem<Trait_*(T)>
    {
        using Trait = Trait_;
    };

    template<class Node_>
    struct TraitNodeInterface
    {
        using Node = Node_;
        using DefaultInterface = Node_;
        static_assert(HasNodeTraits<Node>, "Node passed to Traits has no Traits defined");
    };

    template<class Node_, class Interface_>
    struct TraitNodeInterface<Node_(Interface_)>
    {
        using Node = Node_;
        using DefaultInterface = Interface_;
        static_assert(HasNodeTraits<Node>, "Node passed to Traits has no Traits defined");
    };

    template<class TraitsT, class Trait>
    concept TraitsHasTrait = IsTrait<Trait> and requires (Trait trait, LinkExact<Trait> linkExact) {
        { TraitsT::resolveTrait(trait, linkExact) } -> IsResolvedTrait;
    };

    template<class Trait>
    struct TraitsHasTraitPred
    {
        template<class TraitsT>
        static constexpr bool value = TraitsHasTrait<TraitsT, Trait>;
    };

    template<class Trait>
    struct NodeTraitsHasTraitPred
    {
        template<class Node>
        static constexpr bool value = TraitsHasTrait<typename Node::Traits, Trait>;
    };

    template<class DefaultResolver, class... TraitTs>
    struct Traits;

    struct TraitsDefault;

    struct TraitsOpenDefault;

    template<template<class> class TraitTemplate>
    struct TraitsTemplateDefault;

} // namespace detail

ARC_MODULE_EXPORT
template<class... TraitTs>
using Traits = detail::Traits<detail::TraitsDefault, TraitTs...>;

ARC_MODULE_EXPORT
template<class... TraitTs>
using TraitsOpen = detail::Traits<detail::TraitsOpenDefault, TraitTs...>;

ARC_MODULE_EXPORT
template<template<class> class TraitTemplate, class... TraitTs>
using TraitsTemplate = detail::Traits<detail::TraitsTemplateDefault<TraitTemplate>, TraitTs...>;

} // namespace arc


#endif // INCLUDE_ARC_TRAITS_FWD_HPP
