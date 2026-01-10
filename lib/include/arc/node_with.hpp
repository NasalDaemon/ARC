#ifndef INCLUDE_ARC_NODE_WITH_HPP
#define INCLUDE_ARC_NODE_WITH_HPP

#include "arc/depends.hpp"
#include "arc/node_with_fwd.hpp"
#include "arc/node.hpp"
#include "arc/traits_fwd.hpp"

namespace arc::detail {

template<class NodeBase, class... TraitTs>
struct AsTrait
    : TraitsItem<TraitTs>::Trait::Meta::Converter...
    , TraitsItem<TraitTs>::Trait::Meta::NamedMethods...
{
    using Traits = arc::Traits<TraitTs...>;
};
template<HasNodeTraits NodeBase>
struct AsTrait<NodeBase>
{
    using Traits = NodeBase::Traits;
};
template<class NodeBase>
struct AsTrait<NodeBase>
{
    using Traits = arc::NoTraits;
};

template<class... DependTraits>
struct Resolve
    : DependsTrait<DependTraits>::Meta::Resolver...
{
    using Depends = arc::Depends<DependTraits...>;
};

template<>
struct Resolve<void*>
{
    using Depends = DependsImplicitly;
};

template<class Node, class... T>
struct AssertNoImpl
{
    static_assert(alwaysFalse<Node, T...>, "Cannot use Impl more than once when building a node");
};

template<class Node, class... T>
struct AssertNoUses
{
    static_assert(alwaysFalse<Node, T...>, "Cannot use Uses more than once when building a node");
};

template<IsNodeBase Node, class... DependTraits, class... TraitTs>
struct NodeWith<Node, void(DependTraits...), TraitTs...>
    : Node
    , Resolve<DependTraits...>
    , AsTrait<Node, TraitTs...>
{
    using Depends = Resolve<DependTraits...>::Depends;
    using Traits = AsTrait<Node, TraitTs...>::Traits;

    template<class... Ts>
    using Impl = AssertNoImpl<Node, Ts...>;

    template<detail::IsDependsItem... Ts>
    using Uses = AssertNoUses<Node, Ts...>;
};

}


#endif // INCLUDE_ARC_NODE_WITH_HPP
