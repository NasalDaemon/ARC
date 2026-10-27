#ifndef INCLUDE_ARC_NODES_COLLECTION_FWD_HPP
#define INCLUDE_ARC_NODES_COLLECTION_FWD_HPP

#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"

namespace arc {

ARC_MODULE_EXPORT
template<IsNodeHandle NodeHandle, class ID_, class Policy>
struct Collection;

namespace detail {
    template<class NodeHandle>
    inline constexpr bool isCollection = false;
    template<IsNodeHandle NodeHandle, class ID_, class Policy>
    inline constexpr bool isCollection<Collection<NodeHandle, ID_, Policy>> = true;
}

} // namespace arc

#endif // INCLUDE_ARC_NODES_COLLECTION_FWD_HPP
