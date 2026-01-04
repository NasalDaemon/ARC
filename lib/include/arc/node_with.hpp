#ifndef INCLUDE_ARC_NODE_WITH_HPP
#define INCLUDE_ARC_NODE_WITH_HPP

#include "arc/depends.hpp"
#include "arc/node_with_fwd.hpp"
#include "arc/node.hpp"
#include "arc/traits_fwd.hpp"

namespace arc::detail {

template<class... TraitTs>
struct UnboundTraits
{
    template<class OtherNode, template<class> class NewGetContext, class... ExtraTraits>
    using Rebind = Traits<OtherNode, NewGetContext, detail::TraitsDefault, TraitTs..., ExtraTraits...>;
};

template<class... TraitTs>
struct AsTrait
    : TraitsItem<TraitTs>::Trait::Meta::Converter...
    , TraitsItem<TraitTs>::Trait::Meta::NamedMethods...
{
    using Traits = UnboundTraits<TraitTs...>;
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

template<IsNodeBase Node, class... DependTraits, class... TraitTs>
struct NodeWith<Node, void(DependTraits...), TraitTs...>
    : Node
    , Resolve<DependTraits...>
    , AsTrait<TraitTs...>
{
    using Depends = NodeWith::Resolve::Depends;
};

}


#endif // INCLUDE_ARC_NODE_WITH_HPP
