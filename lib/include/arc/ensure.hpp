#ifndef INCLUDE_ARC_ENSURE_HPP
#define INCLUDE_ARC_ENSURE_HPP

#include "arc/count.hpp"
#include "arc/empty_types.hpp"
#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

namespace arc {

ARC_MODULE_EXPORT
template<NodeDependenciesSatisfied T, template<class> class... Pred>
requires (... and Pred<T>::value)
using Ensure = T;

namespace pred {

    ARC_MODULE_EXPORT
    template<IsUnary Node>
    using Unary = std::true_type;

    ARC_MODULE_EXPORT
    template<class Node>
    requires (not IsUnary<Node>)
    using NonUnary = std::true_type;

    ARC_MODULE_EXPORT
    template<class Node>
    requires IsStateless<Node>
    using Stateless = std::true_type;

    ARC_MODULE_EXPORT
    template<NodeHasDepends Node>
    using HasDepends = std::true_type;

}

}

#endif // INCLUDE_ARC_ENSURE_HPP
