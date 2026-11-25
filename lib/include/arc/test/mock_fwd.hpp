#ifndef INCLUDE_ARC_MOCK_FWD_HPP
#define INCLUDE_ARC_MOCK_FWD_HPP

#include "arc/empty_types.hpp"

namespace arc::test {

namespace detail {
    struct MockReturn;
}

ARC_MODULE_EXPORT
struct MockNodeBase;

ARC_MODULE_EXPORT
template<class DefaultTypes = EmptyTypes, class... MockedTraits>
struct MockNode/* : MockNodeBase */;

ARC_MODULE_EXPORT
template<class DefaultTypes>
using MockWithTypes = MockNode<DefaultTypes>;

ARC_MODULE_EXPORT
template<class... MockedTraits>
using MockWithTraits = MockNode<EmptyTypes, MockedTraits...>;

ARC_MODULE_EXPORT
using Mock = MockNode<>;

}

#endif // INCLUDE_ARC_MOCK_FWD_HPP
