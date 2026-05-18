#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <stdexcept>
#include <string_view>
#endif

import arc.tests.contracts;
import arc;

#include "arc/doctest.h"

namespace arc::tests::contracts {

/* arc-begin

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

    // Void return; exercises the void path in invokeMethod
    reset(int x)
        pre (x >= 0)
}

}

arc-end */

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

    void reset(int) const {}
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

    void reset(this auto& self, int x)
    {
        self.getCalculator().reset(x);
    }
};

#define CHECK_CONTRACT_THROWS(expr, expectedExpr, pos) \
    CHECK_THROWS_WITH_AS(expr, "ARC trait contract failure: " #expectedExpr " (test_contracts.cpp:" pos ")", arc::ContractViolation)

TEST_CASE("arc::IgnoreAssertHandler silently discards all violations")
{
    arc::IgnoreAssertHandler handler;
    CHECK_NOTHROW(handler(0, true,  "no violation"));
    CHECK_NOTHROW(handler(0, false, "violation"));
}

TEST_CASE("arc::ThrowAssertHandler throws ContractViolation on false, not on true")
{
    arc::ThrowAssertHandler handler;
    CHECK_NOTHROW(handler(0, true, "no violation"));
    CHECK_THROWS_WITH_AS(handler(0, false, "my message"), "my message", arc::ContractViolation);
}

TEST_CASE("arc::AbortAssertHandler does not throw for a passing value")
{
    arc::AbortAssertHandler handler;
    CHECK_NOTHROW(handler(0, true, "no violation"));
    // Violation case is noexcept and would call std::terminate
}

TEST_CASE("arc::AssumeAssertHandler does not throw for a passing value")
{
    arc::AssumeAssertHandler handler;
    CHECK_NOTHROW(handler(0, true, "no violation"));
    // Violation case invokes std::unreachable() which is undefined behaviour
}

TEST_CASE("arc::ContractViolation is a std::logic_error")
{
    static_assert(std::is_base_of_v<std::logic_error, arc::ContractViolation>);
    // Verify it is catchable as std::logic_error at runtime
    CHECK_THROWS_WITH_AS(arc::ThrowAssertHandler{}(0, false, "test"), "test", arc::ContractViolation);
}

TEST_CASE("arc::contracts: void-returning method exercises invokeMethod void path")
{
    arc::test::Graph<Caller, Provider> graph;

    // Valid: x >= 0
    CHECK_NOTHROW(graph.node->reset(0));
    CHECK_NOTHROW(graph.node->reset(5));

    // Pre-contract fires when x < 0
    CHECK_CONTRACT_THROWS(graph.node->reset(-1), x >= 0, "36:14");
}

TEST_CASE("arc::contracts: pre-contract fires for invalid arguments")
{
    arc::test::Graph<Caller, Provider> graph;

    // Valid calls should not throw
    CHECK(graph.node->add(3, 4) == 7);
    CHECK(graph.node->divide(6, 2) == 3);

    // Pre-contract fires when a < 0
    CHECK_CONTRACT_THROWS(graph.node->add(-1, 4), a*2 >= 0, "26:14");
    // Pre-contract fires when b < 0
    CHECK_CONTRACT_THROWS(graph.node->add(3, -1), b >= 0, "27:14");
    // Pre-contract fires when divisor is zero
    CHECK_CONTRACT_THROWS(graph.node->divide(6, 0), b != 0, "31:14");
}

TEST_CASE("arc::contracts: post-contract fires for invalid return value")
{
    arc::test::Graph<Caller, Provider> graph;

    // Valid: positive dividend and positive divisor
    CHECK(graph.node->divide(6, 2) == 3);

    // Post-contract fires: negative dividend produces negative result
    CHECK_CONTRACT_THROWS(graph.node->divide(-6, 2), r >= 0, "32:15");
}

namespace custom {

// A stateful handler that records violations instead of throwing, allowing the
// call to proceed normally after a contract failure. State is static so it can
// live in a constexpr-constructed handler instance.
struct RecordingHandler : arc::AssertHandlerBase
{
    static inline int         violationCount = 0;
    static inline const char* lastMessage    = nullptr;

    constexpr void operator()(auto const&, bool value, const char* message) const
    {
        if (not value)
        {
            ++violationCount;
            lastMessage = message;
        }
    }
};

struct Root
{
    using ArcContractAssertHandler = RecordingHandler;
};

} // namespace custom

namespace ignore {

struct Root
{
    using ArcContractAssertHandler = arc::IgnoreAssertHandler;
};

} // namespace ignore

TEST_CASE("arc::contracts: custom ArcContractAssertHandler records violations without throwing")
{
    custom::RecordingHandler::violationCount = 0;
    custom::RecordingHandler::lastMessage    = nullptr;

    arc::test::Graph<Caller, Provider, custom::Root> graph;

    // Valid calls do not trigger the handler
    CHECK(graph.node->add(3, 4) == 7);
    CHECK(custom::RecordingHandler::violationCount == 0);

    // Pre-contract violation: a < 0
    CHECK(graph.node->add(-1, 4) == 3);
    CHECK(custom::RecordingHandler::violationCount == 1);
    CHECK(std::string_view{custom::RecordingHandler::lastMessage}.contains("a*2 >= 0"));

    // Pre-contract violation: b < 0
    CHECK(graph.node->add(3, -1) == 2);
    CHECK(custom::RecordingHandler::violationCount == 2);
    CHECK(std::string_view{custom::RecordingHandler::lastMessage}.contains("b >= 0"));

    // Post-contract violation: negative result from divide
    CHECK(graph.node->divide(-6, 2) == -3);
    CHECK(custom::RecordingHandler::violationCount == 3);
    CHECK(std::string_view{custom::RecordingHandler::lastMessage}.contains("r >= 0"));
}

TEST_CASE("arc::contracts: IgnoreAssertHandler proceeds silently past all violations")
{
    arc::test::Graph<Caller, Provider, ignore::Root> graph;

    // Pre-contract violations proceed normally
    CHECK(graph.node->add(-1, 4) == 3);
    CHECK(graph.node->add(3, -1) == 2);

    // Post-contract violation proceeds normally
    CHECK(graph.node->divide(-6, 2) == -3);

    // Void path: pre-contract violation proceeds normally
    CHECK_NOTHROW(graph.node->reset(-1));
}

} // namespace arc::tests::contracts
