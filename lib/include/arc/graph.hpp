#ifndef INCLUDE_ARC_GRAPH_HPP
#define INCLUDE_ARC_GRAPH_HPP

#include "arc/context_fwd.hpp"
#include "arc/cluster.hpp"
#include "arc/macros.hpp"
#include "arc/static_assert.hpp"
#include "arc/node_fwd.hpp"

#if !ARC_IMPORT_STD
#include <concepts>
#include <functional>
#include <type_traits>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<IsNodeHandle Cluster, class Root = void>
requires IsCluster<typename ToNodeWrapper<Cluster>::template Node<RootContext<Root, Cluster>>>
using Graph = ToNodeWrapper<Cluster>::template Node<RootContext<Root, Cluster>>;

ARC_MODULE_EXPORT
template<std::invocable F>
constexpr std::invoke_result_t<F> constructGraph(F f)
{
    using Result = std::invoke_result_t<F>;
    ARC_STATIC_ASSERT_INLINE(std::derived_from<Result, Cluster>)("Graph root must be a cluster",
        (root, arc::typeName<Result>));
    Result result = std::invoke(f);
    result.onConstructed();
    return result;
}

} // namespace arc


#endif // INCLUDE_ARC_GRAPH_HPP
