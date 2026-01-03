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
struct WithTraits : TraitsItem<TraitTs>::Trait::Meta::Converter...
{
    using Traits = UnboundTraits<TraitTs...>;
};
template<>
struct WithTraits<>
{};

template<class... DependTraits>
struct WithDepends : DependsTrait<DependTraits>::Meta::Resolver...
{
    using Depends = arc::Depends<DependTraits...>;
};
template<>
struct WithDepends<>
{
    using Depends = DependsImplicitly;
};

template<IsNodeBase Node, class... DependTraits, class... TraitTs>
struct NodeWith<Node, void(DependTraits...), TraitTs...>
    : Node
    , WithDepends<DependTraits...>
    , WithTraits<TraitTs...>
{
    using Depends = NodeWith::WithDepends::Depends;
};

}


#endif // INCLUDE_ARC_NODE_WITH_HPP
