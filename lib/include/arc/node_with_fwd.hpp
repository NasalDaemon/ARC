#ifndef INCLUDE_ARC_NODE_WITH_FWD_HPP
#define INCLUDE_ARC_NODE_WITH_FWD_HPP

#include "arc/depends.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"
#include "arc/no_trait.hpp"

namespace arc {

namespace detail {
    template<IsNodeBase Node, class... Traits>
    struct NodeWith;

    struct DisableBuild
    {
        template<class... T>
        struct AssertNoBuild
        {
            static_assert(alwaysFalse<T...>,
                "YourNode::Impl/YourNode::Uses is disabled for YourNode; "
                "use arc::Build<YourNode>/arc::Uses<YourNode, ...>/arc::Impl<YourNode, ...> instead");
        };

        template<class... Traits>
        using Impl = AssertNoBuild<Traits...>;

        template<detail::IsDependsItem... DependTraits>
        using Uses = AssertNoBuild<DependTraits...>;
    };
}

ARC_MODULE_EXPORT
template<class Node, class... Params>
struct Build;

ARC_MODULE_EXPORT
template<IsNodeBase Node, detail::IsDependsItem... Traits>
requires (sizeof...(Traits) > 0)
using Uses = detail::NodeWith<Node, void(Traits...)>;

ARC_MODULE_EXPORT
template<IsNodeBase Node, class... Traits>
requires (sizeof...(Traits) > 0)
using Impl = detail::NodeWith<Node, void(), Traits...>;

template<class Node>
struct Build<Node> final
{
    template<class... Traits>
    requires (sizeof...(Traits) > 0)
    using Impl = Build<Node, void(), Traits...>;

    template<detail::IsDependsItem... DependTraits>
    requires (sizeof...(DependTraits) > 0)
    using Uses = Build<Node, void(DependTraits...)>;
};

template<class Node, detail::IsDependsItem... DependTraits>
struct Build<Node, void(DependTraits...)> final
{
    template<class... Traits>
    requires (sizeof...(Traits) > 0)
    using Impl = detail::NodeWith<Node, void(DependTraits...), Traits...>;

    using NoTraits = detail::NodeWith<Node, void(DependTraits...), arc::NoTraits>;
};

template<class Node, class... Traits>
struct Build<Node, void(), Traits...> final
{
    template<detail::IsDependsItem... DependTraits>
    requires (sizeof...(DependTraits) > 0)
    using Uses = detail::NodeWith<Node, void(DependTraits...), Traits...>;

    using NoDeps = detail::NodeWith<Node, void(), Traits...>;
};

ARC_MODULE_EXPORT
template<detail::IsDependsItem... Traits>
requires (sizeof...(Traits) > 0)
using NodeUses = Uses<Node, Traits...>;

ARC_MODULE_EXPORT
template<class... Traits>
requires (sizeof...(Traits) > 0)
using NodeImpl = Impl<Node, Traits...>;

}


#endif // INCLUDE_ARC_NODE_WITH_FWD_HPP
