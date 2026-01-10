#ifndef INCLUDE_ARC_NODE_WITH_HPP
#define INCLUDE_ARC_NODE_WITH_HPP

#include "arc/depends.hpp"
#include "arc/node_with_fwd.hpp"
#include "arc/node.hpp"
#include "arc/traits_fwd.hpp"

namespace arc::detail {

template<class... TraitTs>
struct AsTrait
    : TraitsItem<TraitTs>::Trait::Meta::Converter...
    , TraitsItem<TraitTs>::Trait::Meta::NamedMethods...
{
    using Traits = arc::Traits<TraitTs...>;
};
template<>
struct AsTrait<>
{};

template<class... DependTraits>
struct Resolve
    : DependsTrait<DependTraits>::Meta::Resolver...
{
    using Depends = arc::Depends<DependTraits...>;
};
template<>
struct Resolve<>
{
    using Depends = Node::Depends;
};

template<class... T>
struct AssertNoImpl
{
    static_assert(alwaysFalse<T...>, "Cannot use Impl more than once when building a node");
};

template<class... T>
struct AssertNoUses
{
    static_assert(alwaysFalse<T...>, "Cannot use Uses more than once when building a node");
};

template<IsNodeBase Node, class... DependTraits, class... TraitTs>
struct NodeWith<Node, void(DependTraits...), TraitTs...>
    : Node
    , Resolve<DependTraits...>
    , AsTrait<TraitTs...>
{
    using Depends = NodeWith::Resolve::Depends;

    template<class... Ts>
    using Impl = AssertNoImpl<Ts...>;

    template<detail::IsDependsItem... Ts>
    using Uses = AssertNoUses<Ts...>;
};

}


#endif // INCLUDE_ARC_NODE_WITH_HPP
