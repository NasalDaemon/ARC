#ifndef INCLUDE_ARC_NODE_WITH_FWD_HPP
#define INCLUDE_ARC_NODE_WITH_FWD_HPP

#include "arc/depends.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"

namespace arc {

namespace detail {
    template<IsNodeBase Node, class... Traits>
    struct NodeWith;

    struct DisableBuild
    {
        template<class... T>
        struct AssertNoBuild
        {
            static_assert(alwaysFalse<T...>, "YourNode::Build is disabled for YourNode; use arc::Build<YourNode> instead");
        };

        template<class... Traits>
        using WithTraits = AssertNoBuild<Traits...>;

        template<detail::IsDependsItem... DependTraits>
        using WithDepends = AssertNoBuild<DependTraits...>;
    };
}

ARC_MODULE_EXPORT
template<class Node, class... Params>
struct Build;

ARC_MODULE_EXPORT
template<IsNodeBase Node, detail::IsDependsItem... Traits>
requires (sizeof...(Traits) > 0)
using WithDepends = detail::NodeWith<Node, void(Traits...)>;

ARC_MODULE_EXPORT
template<IsNodeBase Node, class... Traits>
requires (sizeof...(Traits) > 0)
using WithTraits = detail::NodeWith<Node, void(), Traits...>;

template<class Node>
struct Build<Node>
{
    template<class... Traits>
    requires (sizeof...(Traits) > 0)
    using WithTraits = Build<Node, void(), Traits...>;

    template<detail::IsDependsItem... DependTraits>
    requires (sizeof...(DependTraits) > 0)
    using WithDepends = Build<Node, void(DependTraits...)>;
};

template<class Node, detail::IsDependsItem... DependTraits>
struct Build<Node, void(DependTraits...)>
{
    template<class... Traits>
    requires (sizeof...(Traits) > 0)
    using WithTraits = detail::NodeWith<Node, void(DependTraits...), Traits...>;
};

template<class Node, class... Traits>
struct Build<Node, void(), Traits...>
{
    template<detail::IsDependsItem... DependTraits>
    requires (sizeof...(DependTraits) > 0)
    using WithDepends = detail::NodeWith<Node, void(DependTraits...), Traits...>;
};

ARC_MODULE_EXPORT
template<detail::IsDependsItem... Traits>
requires (sizeof...(Traits) > 0)
using NodeWithDepends = WithDepends<Node, Traits...>;

ARC_MODULE_EXPORT
template<class... Traits>
requires (sizeof...(Traits) > 0)
using NodeWithTraits = WithTraits<Node, Traits...>;

}


#endif // INCLUDE_ARC_NODE_WITH_FWD_HPP
