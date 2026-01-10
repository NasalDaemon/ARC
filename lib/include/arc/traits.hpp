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

        template<class Node>
        using AddDefaultTypes = Trait*(typename Node::Types);
    };

    template<class Trait, class Interface>
    struct TraitResolver<Trait(Interface)>
    {
        static auto resolveTrait(Trait, LinkExact<Trait>) -> ResolvedTrait<Interface, void>;

        template<MatchesTrait<Trait> T>
        static auto resolveTrait(T, LinkPriorityMin) -> ResolvedTrait<Interface, void>;

        template<class Node>
        using AddDefaultTypes = Trait(Interface, typename Node::Types);
    };

    template<class Trait, class Interface, class Types>
    struct TraitResolver<Trait(Interface, Types)>
    {
        static auto resolveTrait(Trait, LinkExact<Trait>) -> ResolvedTrait<Interface, Types>;

        template<MatchesTrait<Trait> T>
        static auto resolveTrait(T, LinkPriorityMin) -> ResolvedTrait<Interface, Types>;

        // Already have explicit types
        template<class Node>
        using AddDefaultTypes = Trait(Interface, Types);
    };

    template<class Types, class Trait>
    struct TraitResolver<Trait*(Types)>
    {
        static auto resolveTrait(Trait, LinkExact<Trait>) -> ResolvedTrait<void, Types>;

        template<MatchesTrait<Trait> T>
        static auto resolveTrait(T, LinkPriorityMin) -> ResolvedTrait<void, Types>;

        // Already have explicit types
        template<class Node>
        using AddDefaultTypes = Trait*(Types);
    };

    template<class T, class Node>
    auto nodeIfVoid() -> T;
    template<class T, class Node>
    requires std::is_void_v<T>
    auto nodeIfVoid() -> Node;

    template<class T, class Node>
    using NodeIfVoid = decltype(nodeIfVoid<T, Node>());

    template<class T, class Node>
    auto nodeTypesIfVoid() -> T;
    template<class T, class Node>
    requires std::is_void_v<T>
    auto nodeTypesIfVoid() -> Node::Types;

    template<class T, class Node>
    using NodeTypesIfVoid = decltype(nodeTypesIfVoid<T, Node>());

    template<class DefaultResolver, class... TraitTs>
    struct Traits : DefaultResolver, TraitResolver<TraitTs>...
    {
        using DefaultResolver::resolveTrait;
        using TraitResolver<TraitTs>::resolveTrait...;

        template<class Trait>
        static constexpr bool HasTrait = TraitsHasTrait<Traits, Trait>;

        template<class Node>
        using AddDefaultTypes = Traits<
            DefaultResolver,
            typename TraitResolver<TraitTs>::template AddDefaultTypes<Node>...
        >;

    private:
        template<class Trait>
        requires TraitsHasTrait<Traits, Trait>
        using Resolve = decltype(Traits::resolveTrait(std::declval<Trait>(), std::declval<LinkExact<Trait>>()));

    public:
        template<class Trait, class Node>
        using ResolveInterface = TraitImplementation<Node, NodeIfVoid<typename Resolve<Trait>::Interface, Node>>;

        template<class Trait, class Node>
        using ResolveTypes = NodeTypesIfVoid<typename Resolve<Trait>::Types, Node>;
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

    template<class Traits>
    struct CombineTraits<Traits>
    {
        using type = Traits;
    };

    template<class DR1, class DR2, class... TraitTs1, class... TraitTs2>
    struct CombineTraits<Traits<DR1, TraitTs1...>, Traits<DR2, TraitTs2...>>
    {
        static_assert(std::is_same_v<DR1, DR2>, "Cannot combine Traits with different DefaultResolvers");
        static_assert(std::is_same_v<DR1, TraitsDefault> or std::is_same_v<DR1, TraitsOpenDefault>,
            "May only combine arc::Traits<...> or arc::TraitsOpen<...>");
        using type = Traits<DR1, TraitTs1..., TraitTs2...>;
    };

    template<class T1, class T2, class T3, class... Ts>
    struct CombineTraits<T1, T2, T3, Ts...>
    {
        using type = CombineTraits<typename CombineTraits<T1, T2>::type, typename CombineTraits<T3, Ts...>::type>::type;
    };

} // namespace arc::detail


#endif // INCLUDE_ARC_TRAITS_HPP
