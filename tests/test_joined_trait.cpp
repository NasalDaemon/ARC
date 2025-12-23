#include <doctest/doctest.h>
#include "arc/macros.hpp"

import arc.tests.joined_trait;
import arc;

/* arc-embed-begin

export module arc.tests.joined_trait;

namespace arc::tests::joined_trait::trait {

trait A
{
    getA() const -> int
}

trait B
{
    getB() const -> int
}

trait AB = A + B

}

cluster arc::tests::joined_trait::TestCluster [Root]
{
    a = Root::A
    b = Root::B
    ab = Root::AB
    aandb = Root::AandB

    [trait::A]
    .., ab --> a
    aandb --> ab
    [trait::B]
    .., ab --> b
    aandb --> ab
    [trait::AB]
    .. --> ab
}

arc-embed-end */

namespace arc::tests::joined_trait {

struct Root
{
    struct A : arc::Node
    {
        using Traits = arc::Traits<A, trait::A>;

        int impl(trait::A::getA) const
        {
            return 42;
        }
    };

    struct B : arc::Node
    {
        using Traits = arc::Traits<B, trait::B>;

        int impl(trait::B::getB) const
        {
            return 84;
        }
    };

    struct AB : arc::Node
    {
        using Depends = arc::Depends<trait::A, trait::B>;
        using Traits = arc::Traits<AB, trait::AB>;

        int impl(this auto const& self, trait::AB::getA)
        {
            return self.getNode(trait::a).getA();
        }

        int impl(this auto const& self, trait::AB::getB)
        {
            return self.getNode(trait::b).getB();
        }
    };

    struct AandB : arc::Node
    {
        using Depends = arc::Depends<trait::A, trait::B>;
        using Traits = arc::Traits<AandB, trait::A, trait::B>;

        int impl(this auto const& self, trait::AB::getA)
        {
            return self.getNode(trait::a).getA();
        }

        int impl(this auto const& self, trait::AB::getB)
        {
            return self.getNode(trait::b).getB();
        }
    };
};

TEST_CASE("arc::JoinedTrait")
{
    arc::Graph<TestCluster, Root> g;

    CHECK(42 == g.asTrait(trait::a).getA());
    CHECK(42 == g.asTrait(trait::aB).getA());
    CHECK(84 == g.asTrait(trait::b).getB());
    CHECK(84 == g.asTrait(trait::aB).getB());

    CHECK(42 == g.ab.asTrait(trait::a).getA());
    CHECK(42 == g.ab.asTrait(trait::aB).getA());
    CHECK(84 == g.ab.asTrait(trait::b).getB());
    CHECK(84 == g.ab.asTrait(trait::aB).getB());

    CHECK(42 == g.aandb.asTrait(trait::a).getA());
    CHECK(84 == g.aandb.asTrait(trait::b).getB());
    static_assert(not g.aandb.hasTrait(trait::aB));
}

}
