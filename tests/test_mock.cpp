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
    REQUIRE(TypeId::of<int&>() != TypeId::of<const int&>());

    arc::test::Graph<MockTestNode> g;
    int i = 101;

    CHECK(not g.mocks->callTrackingEnabled());
    REQUIRE_THROWS(g.mocks->methodCallCount(trait::Trait::takesNothing{}));
    REQUIRE_THROWS(g.mocks->traitCallCount(trait::trait));
    REQUIRE_THROWS(g.mocks->definitionCallCount<trait::Trait::takesNothing>());

    g.mocks->setThrowIfMissing();
    g.mocks->enableCallTracking();
    REQUIRE(g.mocks->callTrackingEnabled());
    REQUIRE(0 == g.mocks->methodCallCount(trait::Trait::takesNothing{}));
    REQUIRE(0 == g.mocks->traitCallCount(trait::trait));
    REQUIRE(0 == g.mocks->methodCallCount(trait::Trait::takesInt{}));

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

    CHECK(1 == g.mocks->methodCallCount(trait::Trait::takesNothing{}));
    CHECK(1 == g.mocks->methodCallCount(trait::Trait::takesInt{}));
    CHECK(1 == g.mocks->methodCallCount(trait::Trait::returnsRef{}));
    CHECK(3 == g.mocks->traitCallCount(trait::trait));

    g.mocks->resetTrackingAndDefinitions();
    // Mode is preserved
    REQUIRE(g.mocks->callTrackingEnabled());
    REQUIRE(g.mocks->throwsIfMissing());
    REQUIRE(not g.mocks->returnsDefault());

    g.mocks->setReturnDefault();
    REQUIRE(not g.mocks->throwsIfMissing());
    REQUIRE(g.mocks->returnsDefault());

    CHECK(0 == g.mocks->methodCallCount(trait::Trait::takesNothing{}));
    CHECK(0 == g.mocks->methodCallCount(trait::Trait::takesInt{}));
    CHECK(0 == g.mocks->methodCallCount(trait::Trait::returnsRef{}));
    CHECK(0 == g.mocks->traitCallCount(trait::trait));

    CHECK(0 == g.node->testNothing());
    CHECK(0 == g.node->testInt(8));

    CHECK(1 == g.mocks->methodCallCount(trait::Trait::takesNothing{}));
    CHECK(1 == g.mocks->methodCallCount(trait::Trait::takesInt{}));
    CHECK(2 == g.mocks->traitCallCount(trait::trait));

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
