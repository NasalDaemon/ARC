#include <doctest/doctest.h>
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <typeinfo>
#include <vector>
#endif

import arc;

namespace arc::tests::combine {

namespace trait {

    struct A : arc::UnconstrainedTrait
    {
        #define ARC_METHODS_A(TAG) \
            TAG(a) \
            TAG(c)
        ARC_METHODS(A)
    } inline constexpr a{};

    struct B : arc::UnconstrainedTrait
    {
        #define ARC_METHODS_B(TAG) \
            TAG(b) \
            TAG(c)
        ARC_METHODS(B)
    } inline constexpr b{};

    struct C : arc::UnconstrainedTrait
    {
        #define ARC_METHODS_C(TAG) \
            TAG(c)
        ARC_METHODS(C)
    } inline constexpr c{};

} // namespace trait

struct A : arc::Node
{
    using Traits = arc::Traits<A, trait::A>;

    int impl(trait::A::a) { return val; }

    int impl(this auto& self, trait::A::c)
    {
        return self.getNode(trait::c).c();
    }

    int val = 42;
};

struct B : arc::Node
{
    using Traits = arc::Traits<B, trait::B>;

    int impl(trait::B::b) { return val; }

    int impl(this auto& self, trait::B::c)
    {
        return self.getNode(trait::c).c();
    }

    int val = 314;
};

struct C : arc::Node
{
    using Traits = arc::Traits<C, trait::C>;

    int impl(trait::C::c) { return val; }

    int val = 99;
};

TEST_CASE("arc::Combine test doubles")
{
    test::Graph<C, Combine<A, B>> g;
    static_assert(sizeof(g) == 3 * sizeof(int));

    CHECK(42 == g.node.getNode(trait::a).a());
    CHECK(314 == g.node.getNode(trait::b).b());

    CHECK(99 == g.node.getNode(trait::a).c());
    CHECK(99 == g.node.getNode(trait::b).c());
}

TEST_CASE_TEMPLATE("arc::Combine with Mock", Mock, test::Mock<EmptyTypes, trait::B>, Narrow<test::Mock<>, trait::B>)
{
    test::Graph<C, Combine<A, Mock>> g;

    g.mocks.template get<Mock>()->define(
        [](trait::B::b)
        {
            return 22;
        },
        [&](trait::B::c)
        {
            return g.node.asTrait(trait::c).c();
        });

    CHECK(42 == g.node.getNode(trait::a).a());
    CHECK(22 == int(g.node.getNode(trait::b).b()));

    CHECK(99 == g.node.getNode(trait::a).c());
    CHECK(99 == int(g.node.getNode(trait::b).c()));
}

} // namespace arc::tests::combine
