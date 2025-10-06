#ifndef INCLUDE_ARC_MOCK_FWD_HPP
#define INCLUDE_ARC_MOCK_FWD_HPP

#include "arc/empty_types.hpp"

namespace arc::test {

ARC_MODULE_EXPORT
template<class DefaultTypes = EmptyTypes, class... MockedTraits>
struct Mock;

}

#endif // INCLUDE_ARC_MOCK_FWD_HPP
