#ifndef INCLUDE_ARC_TRAITS_FWD_HPP
#define INCLUDE_ARC_TRAITS_FWD_HPP

#include "arc/context_fwd.hpp"
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

    template<class Node_>
    struct TraitNodeInterface
    {
        using Node = Node_;
        using DefaultInterface = Node_;
    };

    template<class Node_, class Interface_>
    struct TraitNodeInterface<Node_(Interface_)>
    {
        using Node = Node_;
        using DefaultInterface = Interface_;
    };

    template<class TraitsT, class Trait>
    concept TraitsHasTrait = IsTrait<Trait> and requires (Trait trait) { { TraitsT::resolveTrait(trait) } -> IsResolvedTrait; };

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

    template<class Node_, template<class> class GetContext_, class DefaultResolver, class... TraitTs>
    struct Traits;

    struct TraitsDefault;

    template<class Node>
    struct TraitsOpenDefault;

    template<template<class> class TraitTemplate>
    struct TraitsTemplateDefault;


} // namespace detail

ARC_MODULE_EXPORT
template<class Node, class... TraitTs>
using Traits = detail::Traits<Node, ContextParameterOf, detail::TraitsDefault, TraitTs...>;

ARC_MODULE_EXPORT
template<class Node, class... Traits>
using TraitsOpen = detail::Traits<Node, ContextParameterOf, detail::TraitsOpenDefault<detail::TraitNodeInterface<Node>>, Traits...>;

ARC_MODULE_EXPORT
template<class Node, template<class> class TraitTemplate, class... Traits>
using TraitsTemplate = detail::Traits<Node, ContextParameterOf, detail::TraitsTemplateDefault<TraitTemplate>, Traits...>;

} // namespace arc


#endif // INCLUDE_ARC_TRAITS_FWD_HPP
