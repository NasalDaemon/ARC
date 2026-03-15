#include <doctest/doctest.h>

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <string_view>
#endif

import arc.tests.contracts;
import arc;

namespace arc::tests::contracts {

/* arc-embed-begin

export module arc.tests.contracts;

namespace arc::tests::contracts {

// Trait with pre- and post-contracts for use across all contract tests
trait Calculator
{
    // Result must equal the sum; both operands must be non-negative
    add(int a, int b) -> int
        pre (a*2 >= 0)
        pre (b >= 0)

    // Divisor must be non-zero; result must be non-negative
    divide(int a, int b) -> int
        pre (b != 0)
        post (r: r >= 0)
}

cluster TestCluster [Root]
{
    caller   = Root::Caller
    provider = Root::Provider

    [Calculator]
    caller --> provider
}

}

arc-embed-end */

struct Root
{
    struct Provider : arc::NodeImpl<Calculator>
    {
        int add(int a, int b) const
        {
            return a + b;
        }

        int divide(int a, int b) const
        {
            return a / b;
        }
    };

    struct Caller : arc::NodeUses<Calculator>
    {
        int add(this auto& self, int a, int b)
        {
            return self.getCalculator().add(a, b);
        }

        int divide(this auto& self, int a, int b)
        {
            return self.getCalculator().divide(a, b);
        }
    };
};

#define CHECK_CONTRACT_THROWS(expr, expectedExpr, pos) \
    CHECK_THROWS_WITH_AS(expr, "ARC trait contract failure: " #expectedExpr " (" __FILE__ ":" pos ")", arc::ContractViolation)

TEST_CASE("arc::contracts: pre-contract fires for invalid arguments")
{
    arc::Graph<cluster::TestCluster, Root> graph;

    // Valid calls should not throw
    CHECK(graph.caller->add(3, 4) == 7);
    CHECK(graph.caller->divide(6, 2) == 3);

    // Pre-contract fires when a < 0
    CHECK_CONTRACT_THROWS(graph.caller->add(-1, 4), a*2 >= 0, "25:14");
    // Pre-contract fires when b < 0
    CHECK_CONTRACT_THROWS(graph.caller->add(3, -1), b >= 0, "26:14");
    // Pre-contract fires when divisor is zero
    CHECK_CONTRACT_THROWS(graph.caller->divide(6, 0), b != 0, "30:14");
}

TEST_CASE("arc::contracts: post-contract fires for invalid return value")
{
    arc::Graph<cluster::TestCluster, Root> graph;

    // Valid: positive dividend and positive divisor
    CHECK(graph.caller->divide(6, 2) == 3);

    // Post-contract fires: negative dividend produces negative result
    CHECK_CONTRACT_THROWS(graph.caller->divide(-6, 2), r >= 0, "31:15");
}

namespace custom {

// A stateful handler that records violations instead of throwing, allowing the
// call to proceed normally after a contract failure. State is static so it can
// live in a constexpr-constructed handler instance.
struct RecordingHandler
{
    static inline int         violationCount = 0;
    static inline const char* lastMessage    = nullptr;

    constexpr void operator()(bool value, const char* message) const
    {
        if (not value)
        {
            ++violationCount;
            lastMessage = message;
        }
    }
};

struct Root : arc::tests::contracts::Root
{
    using ArcContractAssertHandler = RecordingHandler;
};

} // namespace custom

TEST_CASE("arc::contracts: custom ArcContractAssertHandler records violations without throwing")
{
    custom::RecordingHandler::violationCount = 0;
    custom::RecordingHandler::lastMessage    = nullptr;

    arc::Graph<cluster::TestCluster, custom::Root> graph;

    // Valid calls do not trigger the handler
    CHECK(graph.caller->add(3, 4) == 7);
    CHECK(custom::RecordingHandler::violationCount == 0);

    // Pre-contract violation: a < 0
    CHECK(graph.caller->add(-1, 4) == 3);
    CHECK(custom::RecordingHandler::violationCount == 1);
    CHECK(std::string_view{custom::RecordingHandler::lastMessage}.contains("a*2 >= 0"));

    // Pre-contract violation: b < 0
    CHECK(graph.caller->add(3, -1) == 2);
    CHECK(custom::RecordingHandler::violationCount == 2);
    CHECK(std::string_view{custom::RecordingHandler::lastMessage}.contains("b >= 0"));

    // Post-contract violation: negative result from divide
    CHECK(graph.caller->divide(-6, 2) == -3);
    CHECK(custom::RecordingHandler::violationCount == 3);
    CHECK(std::string_view{custom::RecordingHandler::lastMessage}.contains("r >= 0"));
}

} // namespace arc::tests::contracts
