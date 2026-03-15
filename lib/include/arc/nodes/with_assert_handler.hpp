#ifndef INCLUDE_ARC_NODES_WITH_ASSERT_HANDLER_HPP
#define INCLUDE_ARC_NODES_WITH_ASSERT_HANDLER_HPP

#include "arc/nodes/map_info.hpp"

namespace arc {

namespace detail {

    template<class T>
    concept IsAssertHandler = std::invocable<T const, bool, const char*>;

    template<IsAssertHandler AssertHandler>
    struct WithAssertHandler
    {
        template<IsContext Context>
        struct MapInfo : Context::Info
        {
            static constexpr AssertHandler ContractAssert{};
        };
    };

} // namespace detail

ARC_MODULE_EXPORT
template<detail::IsAssertHandler AssertHandler>
struct WithAssertHandler : MapInfo<Node, detail::WithAssertHandler<AssertHandler>> {};

namespace node {
    ARC_MODULE_EXPORT
    using arc::WithAssertHandler;
}

} // namespace arc

#endif // INCLUDE_ARC_NODES_WITH_ASSERT_HANDLER_HPP
