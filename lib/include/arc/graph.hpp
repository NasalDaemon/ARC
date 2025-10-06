#ifndef INCLUDE_ARC_GRAPH_HPP
#define INCLUDE_ARC_GRAPH_HPP

#include "arc/context_fwd.hpp"
#include "arc/cluster.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <functional>
#include <type_traits>
#endif

namespace arc {

namespace detail {
    template<class T>
    auto getRootContext() -> RootContext<T>;
    template<class T>
    requires std::is_void_v<T>
    auto getRootContext() -> NullContext;

    template<class T>
    using GetRootContext = decltype(getRootContext<T>());
}

ARC_MODULE_EXPORT
template<template<class> class RootCluster, class Root = void>
requires std::derived_from<RootCluster<detail::GetRootContext<Root>>, Cluster>
using InlineGraph = RootCluster<detail::GetRootContext<Root>>;

ARC_MODULE_EXPORT
template<IsNodeHandle Cluster, class Root = void>
using Graph = InlineGraph<ToNodeWrapper<Cluster>::template Node, Root>;

ARC_MODULE_EXPORT
template<std::invocable F>
constexpr std::invoke_result_t<F> constructGraph(F f)
{
    using Result = std::invoke_result_t<F>;
    static_assert(std::derived_from<Result, Cluster>);
    Result result = std::invoke(f);
    result.onConstructed();
    return result;
}

} // namespace arc


#endif // INCLUDE_ARC_GRAPH_HPP
