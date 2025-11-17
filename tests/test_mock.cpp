#include <doctest/doctest.h>
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <any>
#include <span>
#include <typeinfo>
#include <vector>
#endif

import arc.tests.mock;
import arc;

/*
arc-embed-begin

export module arc.tests.mock;

trait arc::tests::mock::trait::Trait
{
    takesNothing() const
    takesInt(int i)
    returnsRef() -> int&
}

arc-embed-end
*/

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
    CHECK(0 == takesIntCalls.currentIndex());
    CHECK(1 == takesIntCalls.size());
    CHECK(1 == takesIntCalls.nextMatchingIndex().value());

    // Get first (and only) call
    CHECK(std::tuple(8) == takesIntCalls.popFront().value());

    CHECK(1 == takesIntCalls.lastVisitedIndex().value());
    CHECK(2 == takesIntCalls.currentIndex());
    CHECK(0 == takesIntCalls.size());
    CHECK(not takesIntCalls.nextMatchingIndex().has_value());

    // Visiting last call does not affect state
    CHECK(std::tuple(8) == takesIntCalls.back().value());
    CHECK(2 == takesIntCalls.currentIndex());
    CHECK(1 == takesIntCalls.lastVisitedIndex().value());

    // Iterating again yields no calls
    CHECK(not takesIntCalls.popFront().has_value());
    CHECK(1 == takesIntCalls.lastVisitedIndex().value());
    CHECK(3 == takesIntCalls.currentIndex());
    CHECK(0 == takesIntCalls.size());
    CHECK(not takesIntCalls.nextMatchingIndex().has_value());

    // Visiting last call is still possible and does not affect state
    CHECK(std::tuple(8) == takesIntCalls.back().value());
    CHECK(3 == takesIntCalls.currentIndex());
    CHECK(1 == takesIntCalls.lastVisitedIndex().value());

    // Reset visitor to initial state
    takesIntCalls.reset();
    CHECK(not takesIntCalls.lastVisitedIndex().has_value());
    CHECK(0 == takesIntCalls.currentIndex());
    CHECK(1 == takesIntCalls.size());
    CHECK(1 == takesIntCalls.nextMatchingIndex().value());

    CHECK(1 == g.mocks->methodCallCount<trait::Trait::returnsRef>());
    CHECK(3 == g.mocks->traitCallCount<trait::Trait>());

    g.mocks->resetTrackingAndImpls();
    // Mode is preserved
    REQUIRE(g.mocks->callLoggingEnabled());
    REQUIRE(g.mocks->throwsIfMissing());
    REQUIRE(not g.mocks->returnsDefault());

    CHECK_THROWS_MESSAGE(takesIntCalls.validate(), "CallVisitor: Mock call counting state has been invalidated, please rebind if this is expected");
    takesIntCalls.rebind();
    CHECK_NOTHROW(takesIntCalls.validate());
    CHECK(0 == takesIntCalls.currentIndex());
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

    CHECK(1 == takesIntCalls.size());
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
        CHECK_THROWS_WITH(g.node->testNothing(), "Mock implementation not defined for impl(arc::tests::mock::trait::Trait::takesNothing) const");
        CHECK_THROWS_WITH(g.node->testInt(8), "Mock implementation not defined for impl(arc::tests::mock::trait::Trait::takesInt, int)");
        break;
    case MSVC:
        CHECK_THROWS_WITH(g.node->testNothing(), "Mock implementation not defined for impl(struct arc::tests::mock::trait::Trait::takesNothing) const");
        CHECK_THROWS_WITH(g.node->testInt(8), "Mock implementation not defined for impl(struct arc::tests::mock::trait::Trait::takesInt, int)");
        break;
    }

    g.mocks->define([](trait::Trait::returnsRef) { return 0; });

    // Returning dangling reference throws exception
    CHECK_THROWS_AS(g.node->testRef(), std::bad_any_cast);

    // Storing result allows conversion to reference
    auto refResult = g.mocks->impl(trait::Trait::returnsRef{});
    int& ref1 = refResult;
    int& ref2 = refResult;
    CHECK(&ref1 == &ref2);
    CHECK(ref1 == 0);
    ref1 = 123;
    CHECK(ref1 == 123);
    CHECK(ref2 == 123);
}

}
