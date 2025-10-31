#ifndef INCLUDE_ARC_NODE_FWD_HPP
#define INCLUDE_ARC_NODE_FWD_HPP

#include "arc/detail/concepts.hpp"
#include "arc/empty_types.hpp"
#include "arc/factory.hpp"
#include "arc/macros.hpp"
#include "arc/trait.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <type_traits>
#endif

#define ARC_NODE_USE_PUBLIC_MEMBERS(Node) \
    /* Expose utility functions from the underlying node */ \
    using Traits = Node::Traits; \
    using Depends = Node::Depends; \
    using Environment = Node::Environment; \
    using Node::assertNodeContext; \
    using Node::isUnary; \
    using Node::getNode; \
    using Node::canGetNode; \
    using Node::getGlobal; \
    using Node::asTrait; \
    using Node::hasTrait; \
    /* Expose union and virtual node functions */ \
    using Node::exchangeImpl; \
    /* Expose peer node functions */ \
    using Node::getElementId; \
    using Node::getElementHandle; \

namespace arc {

ARC_MODULE_EXPORT
struct Node;

ARC_MODULE_EXPORT
template<class T>
concept IsNode = std::derived_from<T, Node>;

ARC_MODULE_EXPORT
template<class T>
concept IsNodeWrapper = requires {
    typename detail::TakesUnaryClassTemplate<T::template Node>;
} and IsStateless<T>;

ARC_MODULE_EXPORT
template<IsNode NodeT>
struct WrapNode;

template<class Interface, class Context>
struct WrappedImpl : Interface
{
    using Interface::Interface;

    template<class F>
    explicit constexpr WrappedImpl(Emplace<F> const& f)
        : Interface(f)
    {}

    using Traits = WrapNode<typename Interface::Traits::Node>::template Traits<Context>;
};

namespace detail {
    template<class Trait, class Context>
    void isWrappedImpl(WrappedImpl<Trait, Context> const&);
}

ARC_MODULE_EXPORT
template<class T>
concept IsWrappedImpl = requires (T const& t) { detail::isWrappedImpl(t); };

ARC_MODULE_EXPORT
template<class T>
concept IsNodeHandle = IsNodeWrapper<T> or IsNode<T>;

namespace detail {
    template<IsNodeWrapper T>
    auto toNodeWrapper() -> T;
    template<IsNode T>
    auto toNodeWrapper() -> WrapNode<T>;
}

ARC_MODULE_EXPORT
template<IsNodeHandle T>
using ToNodeWrapper = decltype(detail::toNodeWrapper<T>());

namespace detail {
    template<IsNode Node>
    struct NodeState;

    template<IsNode T>
    auto nodeState() -> NodeState<T>;
    template<class T>
    auto nodeState() -> T;

    template<class T>
    void isNodeState(NodeState<T> const&);
    template<class T>
    concept IsNodeState = requires { detail::isNodeState(std::declval<T const&>()); };

    template<class T>
    using ToNodeState = decltype(nodeState<T>());
}

ARC_MODULE_EXPORT
template<template<class> class NodeTmpl>
struct InlineNode
{
    template<class Context>
    using Node = NodeTmpl<Context>;
};

ARC_MODULE_EXPORT
template<class Node>
concept NodeHasDepends = Node::Depends::isSpecified;

ARC_MODULE_EXPORT
template<class Node, class Trait>
concept NodeDependencyListed = Node::Depends::template dependencyListed<Node, Trait>;

ARC_MODULE_EXPORT
template<class Node, bool Transitive = false>
concept NodeDependenciesSatisfied = requires { typename Node::Depends::template AssertSatisfied<Node, Transitive>; };

namespace detail {

template<class T>
auto getUnderlyingNode() -> T;
template<class T>
requires requires { typename T::Traits::Node; }
auto getUnderlyingNode() -> T::Traits::Node;

template<class T>
using UnderlyingNode = decltype(getUnderlyingNode<T>());

template<class Node>
struct NodeHasUnderlyingNodePred
{
    template<class T>
    static constexpr bool value = std::is_same_v<UnderlyingNode<T>, Node>;
};

}

} // namespace arc


#endif // INCLUDE_ARC_NODE_FWD_HPP
