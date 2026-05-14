#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <any>
#include <span>
#include <typeinfo>
#include <vector>
#endif

import arc.tests.mock;
import arc;

#include "doctest.h"

/* arc-begin

export module arc.tests.mock;

trait arc::tests::mock::Trait
{
    takesNothing() const
    takesInt(int i)
    returnsRef() -> int&
}

arc-end */

namespace arc::tests::mock {

struct MockTestNode : arc::Node
{
    using Traits = arc::Traits<MockTestNode>;

    int testNothing(this auto& self)
    {
        return self.getNode(trait::trait).takesNothing();
    }
    int testInt(this auto& self, int i)
    {
        return self.getNode(trait::trait).takesInt(i);
    }
    int& testRef(this auto& self)
    {
        return self.getNode(trait::trait).returnsRef();
    }
};

TEST_CASE("arc::test::Mock")
{
    REQUIRE(TypeId::of<int&>() != TypeId::of<int&&>());
    REQUIRE(TypeId::of<int&>() != TypeId::of<int const&>());

    arc::test::Graph<MockTestNode> g{.mocks{test::MockParams{}}};
    int i = 101;

    CHECK(not g.mocks->callLoggingEnabled());
    REQUIRE_THROWS(g.mocks->methodCallCount<trait::Trait::takesNothing>());
    REQUIRE_THROWS(g.mocks->traitCallCount<trait::Trait>());
    REQUIRE_THROWS(g.mocks->implCallCount<trait::Trait::takesNothing>());

    g.mocks->setThrowIfMissing();
    g.mocks->logAllCalls();
    REQUIRE(g.mocks->callLoggingEnabled());
    REQUIRE(0 == g.mocks->methodCallCount<trait::Trait::takesNothing>());
    REQUIRE(0 == g.mocks->traitCallCount<trait::Trait>());
    REQUIRE(0 == g.mocks->methodCallCount<trait::Trait::takesInt>());

    g.mocks->define(
        [](trait::Trait::takesNothing)
        {
           return 99;
        },
        [](trait::Trait::takesInt, int i)
        {
            return 99 - i;
        },
        [&](trait::Trait::returnsRef) -> int&
        {
            return i;
        });
    CHECK(99 == g.node->testNothing());
    CHECK(91 == g.node->testInt(8));

    int& ref = g.node->testRef();
    CHECK(101 == i);
    CHECK(101 == ref);
    i = 88;
    CHECK(88 == i);
    CHECK(88 == ref);

    CHECK(1 == g.mocks->methodCallCount<trait::Trait::takesNothing>());
    CHECK(1 == g.mocks->methodCallCount<trait::Trait::takesInt>());

    auto takesIntCalls = g.mocks->visitCallLogs<trait::Trait::takesInt, int>();
    // Initial state
    CHECK(not takesIntCalls.lastVisitedIndex().has_value());
    CHECK(1 == takesIntCalls.currentIndex().value());
    CHECK(1 == takesIntCalls.size());

    // Get first (and only) call
    CHECK(std::tuple(8) == takesIntCalls.popFront().value());

    CHECK(1 == takesIntCalls.lastVisitedIndex().value());
    CHECK(not takesIntCalls.currentIndex().has_value());
    CHECK(0 == takesIntCalls.size());

    // Visiting last call does not affect state
    CHECK(std::tuple(8) == takesIntCalls.back().value());
    CHECK(not takesIntCalls.currentIndex().has_value());
    CHECK(1 == takesIntCalls.lastVisitedIndex().value());

    // Iterating again yields no calls
    CHECK(not takesIntCalls.popFront().has_value());
    CHECK(1 == takesIntCalls.lastVisitedIndex().value());
    CHECK(not takesIntCalls.currentIndex().has_value());
    CHECK(0 == takesIntCalls.size());

    // Visiting last call is still possible and does not affect state
    CHECK(std::tuple(8) == takesIntCalls.back().value());
    CHECK(not takesIntCalls.currentIndex().has_value());
    CHECK(1 == takesIntCalls.lastVisitedIndex().value());

    // Reset visitor to initial state
    takesIntCalls.restart();
    CHECK(not takesIntCalls.lastVisitedIndex().has_value());
    CHECK(1 == takesIntCalls.currentIndex().value());
    CHECK(1 == takesIntCalls.size());

    CHECK(1 == g.mocks->methodCallCount<trait::Trait::returnsRef>());
    CHECK(3 == g.mocks->traitCallCount<trait::Trait>());

    g.mocks->resetTrackingAndImpls();
    // Mode is preserved
    REQUIRE(g.mocks->callLoggingEnabled());
    REQUIRE(g.mocks->throwsIfMissing());
    REQUIRE(not g.mocks->returnsDefault());

    CHECK_THROWS_MESSAGE(takesIntCalls.assertValid(false), "CallVisitor: Mock call counting state has been invalidated, please rebind if this is expected");
    takesIntCalls.rebind();
    CHECK_NOTHROW(takesIntCalls.assertValid(false));
    CHECK_THROWS(takesIntCalls.assertValid(true)); // calls have been evicted
    CHECK(not takesIntCalls.currentIndex().has_value());
    CHECK(takesIntCalls.empty());

    g.mocks->setReturnDefault();
    REQUIRE(not g.mocks->throwsIfMissing());
    REQUIRE(g.mocks->returnsDefault());

    CHECK(0 == g.mocks->methodCallCount<trait::Trait::takesNothing>());
    CHECK(0 == g.mocks->methodCallCount<trait::Trait::takesInt>());
    CHECK(0 == g.mocks->methodCallCount<trait::Trait::returnsRef>());
    CHECK(0 == g.mocks->traitCallCount<trait::Trait>());

    CHECK(0 == g.node->testNothing());
    CHECK(0 == g.node->testInt(8));

    CHECK(1 == g.mocks->methodCallCount<trait::Trait::takesNothing>());
    CHECK(1 == g.mocks->methodCallCount<trait::Trait::takesInt>());
    CHECK(2 == g.mocks->traitCallCount<trait::Trait>());

    REQUIRE(1 == takesIntCalls.size());
    CHECK(std::tuple(8) == takesIntCalls.popFront().value());

    g.mocks->setThrowIfMissing();

    switch (arc::compiler.kind)
    {
    using enum arc::Compiler::Kind;
    case GCC:
        CHECK_THROWS_WITH(g.node->testNothing(), "Mock implementation not defined for impl(arc::tests::mock::trait::Trait@arc.tests.mock::takesNothing) const");
        CHECK_THROWS_WITH(g.node->testInt(8), "Mock implementation not defined for impl(arc::tests::mock::trait::Trait@arc.tests.mock::takesInt, int)");
        break;
    case Clang:
        CHECK_THROWS_WITH(g.node->testNothing(), "Mock implementation not defined for impl(arc::tests::mock::Trait::takesNothing) const");
        CHECK_THROWS_WITH(g.node->testInt(8), "Mock implementation not defined for impl(arc::tests::mock::Trait::takesInt, int)");
        break;
    case MSVC:
        CHECK_THROWS_WITH(g.node->testNothing(), "Mock implementation not defined for impl(struct arc::tests::mock::trait::Trait::takesNothing) const");
        CHECK_THROWS_WITH(g.node->testInt(8), "Mock implementation not defined for impl(struct arc::tests::mock::trait::Trait::takesInt, int)");
        break;
    }

    g.mocks->define([](trait::Trait::returnsRef) { return 0; });

    // Returning dangling reference throws exception
    CHECK_THROWS_WITH(g.node->testRef(), "MockReturn: cannot convert int to " ARC_IF_GCC_ELSE("int&")("int &"));

    // Storing result allows conversion to reference
    auto refResult = g.mocks->impl(trait::Trait::returnsRef{});
    int& ref1 = refResult;
    int& ref2 = refResult;
    CHECK(&ref1 == &ref2);
    CHECK(ref1 == 0);
    ref1 = 123;
    CHECK(ref1 == 123);
    CHECK(ref2 == 123);

    g.mocks->methodReturns<trait::Trait::takesNothing>(101);
    g.mocks->methodReturns<trait::Trait::takesInt>(314);
    g.mocks->methodReturns<trait::Trait::returnsRef>(ref1);

    CHECK(101 == g.node->testNothing());
    CHECK(314 == g.node->testInt(8));

    // Definition takes precedence over stored method result
    CHECK_THROWS_WITH(g.node->testRef(), "MockReturn: cannot convert int to " ARC_IF_GCC_ELSE("int&")("int &"));
    g.mocks->undefine<trait::Trait::returnsRef>();
    CHECK(123 == g.node->testRef());
}

TEST_CASE("arc::test::Mock::methodReturns single value repeats")
{
    arc::test::Graph<MockTestNode> g;
    g.mocks->setThrowIfMissing();

    g.mocks->methodReturns<trait::Trait::takesInt>(42);
    CHECK(42 == g.node->testInt(0));
    CHECK(42 == g.node->testInt(0));
    CHECK(42 == g.node->testInt(0));
    g.mocks->methodReturns<trait::Trait::takesInt>(12);
    CHECK(12 == g.node->testInt(0));
    CHECK(12 == g.node->testInt(0));
}

TEST_CASE("arc::test::Mock::implReturns single value")
{
    arc::test::Graph<MockTestNode> g;
    g.mocks->setThrowIfMissing();

    SUBCASE("repeats the same value")
    {
        g.mocks->implReturns<trait::Trait::takesInt, int>(77);
        CHECK(77 == g.node->testInt(0));
        CHECK(77 == g.node->testInt(0));
        CHECK(77 == g.node->testInt(0));
    }

    SUBCASE("overwrites previous define")
    {
        g.mocks->define([](trait::Trait::takesInt, int i) { return i * 2; });
        CHECK(16 == g.node->testInt(8));
        g.mocks->implReturns<trait::Trait::takesInt, int>(99);
        CHECK(99 == g.node->testInt(8));
        CHECK(99 == g.node->testInt(8));
    }

    SUBCASE("overwrites previous implReturns")
    {
        g.mocks->implReturns<trait::Trait::takesInt, int>(10);
        CHECK(10 == g.node->testInt(0));
        g.mocks->implReturns<trait::Trait::takesInt, int>(20);
        CHECK(20 == g.node->testInt(0));
        CHECK(20 == g.node->testInt(0));
    }
}

TEST_CASE("arc::test::Mock::methodReturnsN")
{
    arc::test::Graph<MockTestNode> g;
    g.mocks->setThrowIfMissing();
    g.mocks->enableCallCounting();

    SUBCASE("returns values in order")
    {
        g.mocks->methodReturnsN<trait::Trait::takesInt>(10, 20, 30);
        CHECK(10 == g.node->testInt(0));
        CHECK(20 == g.node->testInt(0));
        CHECK(30 == g.node->testInt(0));
        CHECK(3 == g.mocks->methodCallCount<trait::Trait::takesInt>());
    }

    SUBCASE("throws when exhausted")
    {
        g.mocks->methodReturnsN<trait::Trait::takesInt>(1, 2);
        CHECK(1 == g.node->testInt(0));
        CHECK(2 == g.node->testInt(0));
        CHECK_THROWS_WITH(g.node->testInt(0), "methodReturnsN: No more return values (called 3 times, but only 2 values were provided)");
    }

    SUBCASE("index increments on each throw")
    {
        g.mocks->methodReturnsN<trait::Trait::takesInt>(1, 2);
        CHECK(1 == g.node->testInt(0));
        CHECK(2 == g.node->testInt(0));
        CHECK_THROWS_WITH(g.node->testInt(0), "methodReturnsN: No more return values (called 3 times, but only 2 values were provided)");
        CHECK_THROWS_WITH(g.node->testInt(0), "methodReturnsN: No more return values (called 4 times, but only 2 values were provided)");
        CHECK_THROWS_WITH(g.node->testInt(0), "methodReturnsN: No more return values (called 5 times, but only 2 values were provided)");
    }

    SUBCASE("erases previous methodReturns")
    {
        g.mocks->methodReturns<trait::Trait::takesInt>(99);
        g.mocks->methodReturnsN<trait::Trait::takesInt>(1, 2);
        CHECK(1 == g.node->testInt(0));
        CHECK(2 == g.node->testInt(0));
        CHECK_THROWS(g.node->testInt(0));
    }

}

TEST_CASE("arc::test::Mock::implReturnsN")
{
    arc::test::Graph<MockTestNode> g;
    g.mocks->setThrowIfMissing();
    g.mocks->enableCallCounting();

    SUBCASE("returns values in order")
    {
        g.mocks->implReturnsN<trait::Trait::takesInt, int>(100, 200);
        CHECK(100 == g.node->testInt(0));
        CHECK(200 == g.node->testInt(0));
        CHECK(2 == g.mocks->implCallCount<trait::Trait::takesInt, int>());
    }

    SUBCASE("throws when exhausted")
    {
        g.mocks->implReturnsN<trait::Trait::takesInt, int>(5, 6);
        CHECK(5 == g.node->testInt(0));
        CHECK(6 == g.node->testInt(0));
        CHECK_THROWS_WITH(g.node->testInt(0), "implReturnsN: No more return values (called 3 times, but only 2 values were provided)");
    }

    SUBCASE("index increments on each throw")
    {
        g.mocks->implReturnsN<trait::Trait::takesInt, int>(5, 6);
        CHECK(5 == g.node->testInt(0));
        CHECK(6 == g.node->testInt(0));
        CHECK_THROWS_WITH(g.node->testInt(0), "implReturnsN: No more return values (called 3 times, but only 2 values were provided)");
        CHECK_THROWS_WITH(g.node->testInt(0), "implReturnsN: No more return values (called 4 times, but only 2 values were provided)");
        CHECK_THROWS_WITH(g.node->testInt(0), "implReturnsN: No more return values (called 5 times, but only 2 values were provided)");
    }

    SUBCASE("erases previous implReturns")
    {
        g.mocks->implReturns<trait::Trait::takesInt, int>(99);
        g.mocks->implReturnsN<trait::Trait::takesInt, int>(11, 22);
        CHECK(11 == g.node->testInt(0));
        CHECK(22 == g.node->testInt(0));
        CHECK_THROWS(g.node->testInt(0));
    }

    SUBCASE("erases previous define for same impl")
    {
        g.mocks->define([](trait::Trait::takesInt, int i) { return i * 2; });
        g.mocks->implReturnsN<trait::Trait::takesInt, int>(50, 60);
        CHECK(50 == g.node->testInt(0));
        CHECK(60 == g.node->testInt(0));
    }
}

} // namespace arc::tests::mock
