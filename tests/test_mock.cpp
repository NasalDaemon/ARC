#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <any>
#include <memory>
#include <optional>
#include <span>
#include <typeinfo>
#include <vector>
#endif

import arc.tests.mock;
import arc;

#include "arc/doctest.h"

/* arc-begin

export module arc.tests.mock;

trait arc::tests::mock::Trait [Types]
{
    type MoveOnly
    type OptionalInt

    takesNothing() const
    takesInt(int i)
    returnsRef() -> int&
    returnsConstRef() const -> int const&
    returnsValue() const -> int
    returnsOptional() const -> Types::OptionalInt
    returnsMoveOnly() const -> Types::MoveOnly
    returnsRvalueRef() const -> int&&
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
    int const& testConstRef(this auto& self)
    {
        return self.getNode(trait::trait).returnsConstRef();
    }
    int testValue(this auto& self)
    {
        return self.getNode(trait::trait).returnsValue();
    }
    std::optional<int> testOptional(this auto& self)
    {
        return self.getNode(trait::trait).returnsOptional();
    }
    std::unique_ptr<int> testMoveOnly(this auto& self)
    {
        return self.getNode(trait::trait).returnsMoveOnly();
    }
    int&& testRvalueRef(this auto& self)
    {
        return self.getNode(trait::trait).returnsRvalueRef();
    }
};

struct MockTypes
{
    using MoveOnly = std::unique_ptr<int>;
    using OptionalInt = std::optional<int>;
};

using MockTestGraph = arc::test::Graph<MockTestNode, arc::test::Mock<MockTypes>>;

TEST_CASE("arc::test::Mock")
{
    REQUIRE(TypeId::of<int&>() != TypeId::of<int&&>());
    REQUIRE(TypeId::of<int&>() != TypeId::of<int const&>());

    MockTestGraph g{.mocks{test::MockParams{}}};
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

TEST_CASE("arc::test::Mock trait call through a trait view")
{
    MockTestGraph g;
    g.mocks->setThrowIfMissing();
    g.mocks->enableCallCounting();

    g.mocks->define([](trait::Trait::takesInt, int i) { return i + 1; });

    // Obtain a TraitView over the mock's Trait implementation and call through it,
    // rather than going through the node-under-test's accessor methods.
    arc::IsTraitViewOf<trait::Trait> auto view = g.mocks.asTrait(trait::trait);

    CHECK(43 == view.takesInt(42));
    CHECK(1 == g.mocks->methodCallCount<trait::Trait::takesInt>());

    // A reference return binds through the view to the def's referent.
    int storage = 7;
    g.mocks->define([&](trait::Trait::returnsRef) -> int& { return storage; });
    int& r = view.returnsRef();
    r = 99;
    CHECK(99 == storage);
    CHECK(1 == g.mocks->methodCallCount<trait::Trait::returnsRef>());
}

TEST_CASE("arc::test::Mock::methodReturns single value repeats")
{
    MockTestGraph g;
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
    MockTestGraph g;
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
    MockTestGraph g;
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
    MockTestGraph g;
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

// Exercises every combination of {value stored, reference stored} against every
// kind of method return type {value, T&, T const&, std::optional<T>, move-only}.
// The conversion must be unambiguous and behave identically on every compiler:
// a reference return bound to a stored value (which would dangle) throws, a
// value return copies, and a converting-constructor return type (std::optional)
// is not confused with its element type.
TEST_CASE("arc::test::Mock MockReturn conversion matrix")
{
    MockTestGraph g;
    g.mocks->setThrowIfMissing();
    int storage = 100;

    SUBCASE("value return: def returns a value -> copied out")
    {
        g.mocks->define([](trait::Trait::returnsValue) { return 7; });
        CHECK(7 == g.node->testValue());
        CHECK(7 == g.node->testValue()); // repeatable
    }
    SUBCASE("value return: def returns a reference -> copied out (referent untouched)")
    {
        g.mocks->define([&](trait::Trait::returnsValue) -> int const& { return storage; });
        CHECK(100 == g.node->testValue());
        storage = 101;
        CHECK(101 == g.node->testValue());
    }

    SUBCASE("lvalue-ref return: def returns a reference -> bound through")
    {
        g.mocks->define([&](trait::Trait::returnsRef) -> int& { return storage; });
        int& r = g.node->testRef();
        r = 55;
        CHECK(55 == storage);
    }
    SUBCASE("lvalue-ref return: def returns a value -> throws (would dangle)")
    {
        g.mocks->define([](trait::Trait::returnsRef) { return 5; });
        CHECK_THROWS(g.node->testRef());
    }

    SUBCASE("const-ref return: def returns a reference -> bound through (the GCC/Clang divergence case)")
    {
        g.mocks->define([&](trait::Trait::returnsConstRef) -> int const& { return storage; });
        int const& r = g.node->testConstRef();
        CHECK(100 == r);
        storage = 77;
        CHECK(77 == r);
    }
    SUBCASE("const-ref return: def returns a non-const reference -> bound through")
    {
        g.mocks->define([&](trait::Trait::returnsConstRef) -> int& { return storage; });
        CHECK(100 == g.node->testConstRef());
    }
    SUBCASE("const-ref return: def returns a value -> throws (would dangle)")
    {
        g.mocks->define([](trait::Trait::returnsConstRef) { return 9; });
        CHECK_THROWS(g.node->testConstRef());
    }

    SUBCASE("optional return: def returns the optional -> not confused with its element type")
    {
        g.mocks->define([](trait::Trait::returnsOptional) { return std::optional<int>{42}; });
        CHECK(std::optional<int>{42} == g.node->testOptional());
    }
    SUBCASE("optional return: empty optional round-trips")
    {
        g.mocks->define([](trait::Trait::returnsOptional) { return std::optional<int>{}; });
        CHECK(not g.node->testOptional().has_value());
    }
    SUBCASE("optional return: via methodReturns")
    {
        g.mocks->methodReturns<trait::Trait::returnsOptional>(std::optional<int>{13});
        CHECK(std::optional<int>{13} == g.node->testOptional());
    }

    SUBCASE("move-only return: def returns a fresh move-only value each call -> moved out")
    {
        g.mocks->define([](trait::Trait::returnsMoveOnly) { return std::make_unique<int>(123); });
        auto p = g.node->testMoveOnly();
        REQUIRE(p != nullptr);
        CHECK(123 == *p);
        auto p2 = g.node->testMoveOnly();
        REQUIRE(p2 != nullptr);
        CHECK(123 == *p2);
    }

    SUBCASE("rvalue-ref return through a trait view: def returns a reference")
    {
        auto view = g.mocks.asTrait(trait::trait);
        g.mocks->define([&](trait::Trait::returnsRvalueRef) -> int&& { return std::move(storage); });
        int&& moved = view.returnsRvalueRef();
        CHECK(100 == moved);
        CHECK(&moved == &storage);
    }

    SUBCASE("stored value on a named MockReturn binds to a reference and persists")
    {
        g.mocks->define([](trait::Trait::returnsRef) { return 0; });
        auto stored = g.mocks->impl(trait::Trait::returnsRef{}); // named lvalue MockReturn holding a value
        int& a = stored;
        int& b = stored;
        CHECK(&a == &b);
        a = 88;
        CHECK(88 == b);
        int copy = stored; // value conversion of the same holder
        CHECK(88 == copy);
        int const& c = stored;
        CHECK(88 == c);
    }
}

} // namespace arc::tests::mock
