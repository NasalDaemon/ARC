#ifndef INCLUDE_ARC_TRAITS_HPP
#define INCLUDE_ARC_TRAITS_HPP

#include "arc/detached.hpp"
#include "arc/macros.hpp"
#include "arc/traits_fwd.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

namespace arc::detail {

    template<class Node, class T>
    auto getTraitImplementation() -> T;

    template<class Node, IsDetachedInterface T>
    auto getTraitImplementation() -> DetachedImpl<Node, T>;

    template<class Node, class T>
    using TraitImplementation = decltype(getTraitImplementation<Node, T>());

    template<class NodeInterface, class Trait>
    struct TraitResolver
    {
        // Exact match is highest priority
        template<ExactlyMatchesTrait<Trait> T>
        static auto resolveTrait(T) -> ResolvedTrait<typename NodeInterface::DefaultInterface, typename NodeInterface::Node::Types>;

        template<MatchesTrait<Trait> T>
        static auto resolveTrait(T) -> ResolvedTrait<typename NodeInterface::DefaultInterface, typename NodeInterface::Node::Types>;
    };

    template<class NodeInterface, class Trait, class Interface>
    struct TraitResolver<NodeInterface, Trait(Interface)>
    {
        template<ExactlyMatchesTrait<Trait> T>
        static auto resolveTrait(T) -> ResolvedTrait<Interface, typename NodeInterface::Node::Types>;

        template<MatchesTrait<Trait> T>
        static auto resolveTrait(T) -> ResolvedTrait<Interface, typename NodeInterface::Node::Types>;
    };

    template<class NodeInterface, class Trait, class Interface, class Types>
    struct TraitResolver<NodeInterface, Trait(Interface, Types)>
    {
        template<ExactlyMatchesTrait<Trait> T>
        static auto resolveTrait(T) -> ResolvedTrait<Interface, Types>;

        template<MatchesTrait<Trait> T>
        static auto resolveTrait(T) -> ResolvedTrait<Interface, Types>;
    };

    template<class NodeInterface, class Types, class Trait>
    struct TraitResolver<NodeInterface, Trait*(Types)>
    {
        template<ExactlyMatchesTrait<Trait> T>
        static auto resolveTrait(T) -> ResolvedTrait<typename NodeInterface::DefaultInterface, Types>;

        template<MatchesTrait<Trait> T>
        static auto resolveTrait(T) -> ResolvedTrait<typename NodeInterface::DefaultInterface, Types>;
    };

    template<class Node_, template<class> class GetContext_, class DefaultResolver, class... TraitTs>
    struct Traits : DefaultResolver, TraitResolver<TraitNodeInterface<Node_>, TraitTs>...
    {
        using Node = TraitNodeInterface<Node_>::Node;
        template<class T = Node>
        using GetContext = GetContext_<T>;

        using DefaultResolver::resolveTrait;
        using TraitResolver<TraitNodeInterface<Node_>, TraitTs>::resolveTrait...;

        template<class Trait>
        static constexpr bool HasTrait = TraitsHasTrait<Traits, Trait>;

    private:
        template<class Trait>
        requires TraitsHasTrait<Traits, Trait>
        using Resolve = decltype(Traits::resolveTrait(std::declval<Trait>()));

    public:
        template<class Trait>
        using ResolveInterface = TraitImplementation<Node, typename Resolve<Trait>::Interface>;

        template<class Trait>
        using ResolveTypes = Resolve<Trait>::Types;
    };

    struct TraitsDefault
    {
        template<class UnmappedTrait>
        static ResolvedTrait<void, void> resolveTrait(UnmappedTrait) = delete;
    };

    template<class NodeInterface>
    struct TraitsOpenDefault
    {
        static auto resolveTrait(auto) -> ResolvedTrait<typename NodeInterface::DefaultInterface, typename NodeInterface::Node::Types>;
    };

    template<template<class> class TraitTemplate>
    struct TraitsTemplateDefault
    {
        template<class Trait>
        static TraitTemplate<Trait> resolveTrait(Trait);
    };


} // namespace arc::detail


#endif // INCLUDE_ARC_TRAITS_HPP
