#include <doctest/doctest.h>
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <any>
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
    arc::test::Graph<MockTestNode> g;
    int i = 101;

    CHECK(0 == g.mocks->methodCallCount(trait::Trait::takesNothing{}));
    CHECK(0 == g.mocks->methodCallCount(trait::Trait::takesInt{}));

    g.mocks->setThrowIfMissing();

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

    g.mocks->reset();

    CHECK(0 == g.mocks->methodCallCount(trait::Trait::takesNothing{}));
    CHECK(0 == g.mocks->methodCallCount(trait::Trait::takesInt{}));
    CHECK(0 == g.mocks->methodCallCount(trait::Trait::returnsRef{}));

    // Default behaviour is to return default value

    CHECK(0 == g.node->testNothing());
    CHECK(0 == g.node->testInt(8));

    CHECK(1 == g.mocks->methodCallCount(trait::Trait::takesNothing{}));
    CHECK(1 == g.mocks->methodCallCount(trait::Trait::takesInt{}));

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
