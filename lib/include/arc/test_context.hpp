#ifndef INCLUDE_ARC_TEST_CONTEXT_HPP
#define INCLUDE_ARC_TEST_CONTEXT_HPP

#include "arc/context_fwd.hpp"

namespace arc::test {

namespace detail {
struct TestContextTag{};
}

ARC_MODULE_EXPORT
template<class Context>
concept IsTestContext = IsContext<Context> and requires { Context::Info::isTestContext(detail::TestContextTag{}); };

} // namespace arc::test


#endif // INCLUDE_ARC_TEST_CONTEXT_HPP
