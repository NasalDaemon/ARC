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

#define ARC_NODE_USE_PUBLIC_MEMBERS(NodeT) \
    using Node = detail::NodeOf<NodeT>; \
    using Traits = NodeT::Traits; \
    using Depends = NodeT::Depends; \
    using Environment = NodeT::Environment; \
    using Types = NodeT::Types; \
    template<class Source, class Node, class Types, class... Keys> \
    using FinaliseTypes = NodeT::template FinaliseTypes<Source, Node, Types, Keys...>; \
    /* Expose utility functions from the underlying node */ \
    using NodeT::assertNodeContext; \
    using NodeT::isUnary; \
    using NodeT::getNode; \
    using NodeT::canGetNode; \
    using NodeT::getGlobal; \
    using NodeT::asTrait; \
    using NodeT::hasTrait; \
    /* Expose union and virtual node functions */ \
    using NodeT::exchangeImpl; \
    /* Expose peer node functions */ \
    using NodeT::getElementId; \
    using NodeT::getElementHandle; \

namespace arc {

ARC_MODULE_EXPORT
struct Node;

namespace detail {
    template<class T>
    requires requires { typename T::Node; } and (not std::is_same_v<typename T::Node, arc::Node>)
    auto getNodeOf() -> T::Node;
    template<class T>
    auto getNodeOf() -> T;

    ARC_MODULE_EXPORT
    template<class T>
    using NodeOf = decltype(getNodeOf<T>());
}

ARC_MODULE_EXPORT
template<class T>
concept IsNode = detail::IsInstanceOf<T, Node>;

namespace detail {
    template<class T>
    inline constexpr bool isNodeBase = false;

    template<>
    inline constexpr bool isNodeBase<Node> = true;

    template<class T>
    concept IsNodeBaseC = isNodeBase<T>;
}

ARC_MODULE_EXPORT
template<class T>
concept IsNodeBase = IsNode<T> and detail::IsNodeBaseC<T>;

ARC_MODULE_EXPORT
template<class T>
concept IsNodeWrapper = requires {
    typename detail::TakesUnaryClassTemplate<T::template Node>;
} and IsStateless<T>;

ARC_MODULE_EXPORT
template<IsNode NodeT>
struct WrapNode;

namespace detail {
    template<class NodeT>
    struct WrappedImplNode
    {};

    template<class NodeT>
    NodeT getWrappedImplNode(WrappedImplNode<NodeT> const&);
}

template<class NodeT, class Interface>
struct WrappedImpl
{
    template<class Context>
    struct Node : Interface, detail::WrappedImplNode<NodeT>
    {
        using IsWrappedImpl = void;
        using Interface::Interface;

        template<class F>
        explicit constexpr Node(Emplace<F>&& f)
            : Interface(std::move(f))
        {}

        using Traits = WrapNode<NodeT>::template Traits<Context>;
    };
};

ARC_MODULE_EXPORT
template<class T>
concept IsWrappedImpl = requires (T const& t) { detail::getWrappedImplNode(t); };

ARC_MODULE_EXPORT
template<class T>
concept IsNodeHandle = IsNodeWrapper<T> or IsNode<T>;

namespace detail {
    template<IsNodeWrapper T>
    auto toNodeWrapper() -> T;
    template<IsNode T>
    auto toNodeWrapper() -> WrapNode<T>;

    template<class T>
    auto toNodeUserClass() -> T;
    template<IsWrappedImpl T>
    auto toNodeUserClass() -> decltype(getWrappedImplNode(std::declval<T const&>()));
    template<class T>
    using NodeUserClass = decltype(toNodeUserClass<T>());
}

ARC_MODULE_EXPORT
template<class T>
using UnderlyingNode = detail::NodeUserClass<detail::NodeOf<T>>;

ARC_MODULE_EXPORT
template<IsNodeHandle T>
using ToNodeWrapper = decltype(detail::toNodeWrapper<T>());

namespace detail {
    template<IsNode Node>
    struct NodeState;

    template<IsNode T>
    auto nodeState() -> NodeState<T>;
    // Clusters should not be wrapped like Nodes, so member nodes stay public
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

} // namespace arc


#endif // INCLUDE_ARC_NODE_FWD_HPP
