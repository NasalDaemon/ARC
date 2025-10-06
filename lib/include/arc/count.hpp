#ifndef INCLUDE_ARC_COUNT_HPP
#define INCLUDE_ARC_COUNT_HPP

#include "arc/macros.hpp"
#include "arc/node_fwd.hpp"
#if !ARC_IMPORT_STD
#include <cstdint>
#include <utility>
#endif


namespace arc {

ARC_MODULE_EXPORT
constexpr std::size_t nodeCount(auto const& node)
{
    std::size_t count = 0;
    node.visit([&](IsNode auto&) { ++count; });
    return count;
}

ARC_MODULE_EXPORT
template<class T>
concept IsUnary = T::isUnary();

} // namespace arc


#endif // INCLUDE_ARC_COUNT_HPP
