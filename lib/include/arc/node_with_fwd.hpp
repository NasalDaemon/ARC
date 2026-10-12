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

    template<class T>
    concept IsQualified = std::is_pointer_v<T>;

    template<class T>
    auto getNamedTrait() -> T;
    template<IsQualified Q>
    auto getNamedTrait() -> Q;

    template<class T>
    auto getQualifiedTrait() -> T*;
    template<IsQualified Q>
    auto getQualifiedTrait() -> Q;

    template<class T>
    auto getRawTrait() -> T;
    template<IsQualified Q>
    auto getRawTrait() -> std::remove_pointer_t<Q>;

    template<class TraitT>
    using NamedTrait = decltype(getNamedTrait<TraitT>());
    template<class TraitT>
    using QualifiedTrait = decltype(getQualifiedTrait<TraitT>());
    template<class TraitT>
    using RawTrait = decltype(getRawTrait<TraitT>());
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
using Impl = detail::NodeWith<Node, void(), detail::NamedTrait<Traits>...>;

// Each Build is final, so a node deriving from a chain left unfinished fails at its base, where the compiler's
// note shows the line declaring that Build: the comment on it names the fix.
template<class Node>
struct Build<Node> final // Unfinished node builder: continue with ::Impl<Traits...> and/or ::Uses<Depends...>
{
    template<class... Traits>
    requires (sizeof...(Traits) > 0)
    using Impl = Build<Node, void(), detail::NamedTrait<Traits>...>;

    template<detail::IsDependsItem... DependTraits>
    requires (sizeof...(DependTraits) > 0)
    using Uses = Build<Node, void(DependTraits...)>;
};

template<class Node, detail::IsDependsItem... DependTraits>
struct Build<Node, void(DependTraits...)> final // Unfinished node builder: finish with ::Impl<Traits...>, or ::NoTraits (or use arc::NodeUses<...>)
{
    template<class... Traits>
    requires (sizeof...(Traits) > 0)
    using Impl = detail::NodeWith<Node, void(DependTraits...), detail::NamedTrait<Traits>...>;

    using NoTraits = detail::NodeWith<Node, void(DependTraits...)>;
};

template<class Node, class... Traits>
struct Build<Node, void(), Traits...> final // Unfinished node builder: finish with ::Uses<Depends...>, or ::UsesAny (or use arc::NodeImpl<...>)
{
    template<detail::IsDependsItem... DependTraits>
    requires (sizeof...(DependTraits) > 0)
    using Uses = detail::NodeWith<Node, void(DependTraits...), Traits...>;

    using UsesAny = detail::NodeWith<Node, void(void*), Traits...>;
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
