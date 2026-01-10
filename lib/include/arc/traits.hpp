#ifndef INCLUDE_ARC_TRAITS_HPP
#define INCLUDE_ARC_TRAITS_HPP

#include "arc/detached.hpp"
#include "arc/macros.hpp"
#include "arc/link.hpp"
#include "arc/traits_fwd.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#include <utility>
#endif

namespace arc::detail {

    template<class Node, class T>
    auto getTraitImplementation() -> T;

    template<class Node, IsDetachedInterface T>
    auto getTraitImplementation() -> DetachedImpl<Node, T>;

    template<class Node, class T>
    using TraitImplementation = decltype(getTraitImplementation<Node, T>());

    template<class Trait>
    struct TraitResolver
    {
        // Exact match is highest priority
        static auto resolveTrait(Trait, LinkExact<Trait>) -> ResolvedTrait<void, void>;

        template<MatchesTrait<Trait> T>
        static auto resolveTrait(T, LinkPriorityMin) -> ResolvedTrait<void, void>;
    };

    template<class Trait, class Interface>
    struct TraitResolver<Trait(Interface)>
    {
        static auto resolveTrait(Trait, LinkExact<Trait>) -> ResolvedTrait<Interface, void>;

        template<MatchesTrait<Trait> T>
        static auto resolveTrait(T, LinkPriorityMin) -> ResolvedTrait<Interface, void>;
    };

    template<class Trait, class Interface, class Types>
    struct TraitResolver<Trait(Interface, Types)>
    {
        static auto resolveTrait(Trait, LinkExact<Trait>) -> ResolvedTrait<Interface, Types>;

        template<MatchesTrait<Trait> T>
        static auto resolveTrait(T, LinkPriorityMin) -> ResolvedTrait<Interface, Types>;
    };

    template<class Types, class Trait>
    struct TraitResolver<Trait*(Types)>
    {
        static auto resolveTrait(Trait, LinkExact<Trait>) -> ResolvedTrait<void, Types>;

        template<MatchesTrait<Trait> T>
        static auto resolveTrait(T, LinkPriorityMin) -> ResolvedTrait<void, Types>;
    };

    template<class T, class Node>
    auto nodeIfVoid() -> T;
    template<class T, class Node>
    requires std::is_void_v<T>
    auto nodeIfVoid() -> Node;

    template<class T, class Node>
    auto nodeTypesIfVoid() -> T;
    template<class T, class Node>
    requires std::is_void_v<T>
    auto nodeTypesIfVoid() -> Node::Types;

    template<class DefaultResolver, class... TraitTs>
    struct Traits : DefaultResolver, TraitResolver<TraitTs>...
    {
        using DefaultResolver::resolveTrait;
        using TraitResolver<TraitTs>::resolveTrait...;

        template<class Trait>
        static constexpr bool HasTrait = TraitsHasTrait<Traits, Trait>;

    private:
        template<class Trait>
        requires TraitsHasTrait<Traits, Trait>
        using Resolve = decltype(Traits::resolveTrait(std::declval<Trait>(), std::declval<LinkExact<Trait>>()));

    public:
        template<class Trait, class Node>
        using ResolveInterface = TraitImplementation<Node, decltype(nodeIfVoid<typename Resolve<Trait>::Interface, Node>())>;

        template<class Trait, class Node>
        using ResolveTypes = decltype(nodeTypesIfVoid<typename Resolve<Trait>::Types, Node>());
    };

    struct TraitsDefault
    {
        template<class UnmappedTrait>
        static ResolvedTrait<void, void> resolveTrait(UnmappedTrait, LinkPriorityMin) = delete;
    };

    struct TraitsOpenDefault
    {
        static auto resolveTrait(auto, LinkPriorityMin) -> ResolvedTrait<void, void>;
    };

    template<template<class> class TraitTemplate>
    struct TraitsTemplateDefault
    {
        template<class Trait>
        static TraitTemplate<Trait> resolveTrait(Trait, LinkPriorityMin);
    };


} // namespace arc::detail


#endif // INCLUDE_ARC_TRAITS_HPP
