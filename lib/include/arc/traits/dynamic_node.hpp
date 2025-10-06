#ifndef INCLUDE_ARC_TRAITS_DYNAMIC_NODE_HPP
#define INCLUDE_ARC_TRAITS_DYNAMIC_NODE_HPP

#include "arc/traits/dynamic_node.hxx"

#include "arc/macros.hpp"
#include "arc/union_fwd.hpp"
#include "arc/virtual_fwd.hpp"

namespace arc {

ARC_MODULE_EXPORT
template<class Context>
concept IsDynamicContext = IsContext<Context> and (IsVirtualContext<Context> or IsUnionContext<Context>);

} // namespace arc

#endif // INCLUDE_ARC_TRAITS_DYNAMIC_NODE_HPP
