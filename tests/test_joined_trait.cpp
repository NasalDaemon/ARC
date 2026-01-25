#include <doctest/doctest.h>
#include "arc/macros.hpp"

import arc.tests.joined_trait;
import arc;

/* arc-embed-begin

export module arc.tests.joined_trait;

namespace arc::tests::joined_trait::trait {

trait A
{
    fnA() const -> int
}

trait B
{
    fnB() const -> int
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
        using Traits = arc::Traits<trait::A>;

        int impl(trait::A::fnA) const
        {
            return 42;
        }
    };

    struct B : arc::Node
    {
        using Traits = arc::Traits<trait::B>;

        int impl(trait::B::fnB) const
        {
            return 84;
        }
    };

    struct AB : arc::Node::Impl<trait::AB*>::Uses<trait::A, trait::B>
    {
        int impl(this auto const& self, trait::AB::fnA)
        {
            static_assert(requires { self.asA(); });
            static_assert(requires { self.AsTrait::asA(); });

            static_assert(requires { self.fnA(); });
            static_assert(requires { self.A::fnA(); });
            static_assert(requires { self.AB::fnA(); });
            static_assert(requires { self.Methods::fnA(); });
            static_assert(requires { self.Methods::A::fnA(); });
            static_assert(requires { self.Methods::AB::fnA(); });

            static_assert(requires { self.Resolve::getA(); });
            return self.getA().fnA();
        }

        int impl(this auto const& self, trait::AB::fnB)
        {
            static_assert(requires { self.asB(); });
            static_assert(requires { self.AsTrait::asB(); });

            static_assert(requires { self.fnB(); });
            static_assert(requires { self.B::fnB(); });
            static_assert(requires { self.AB::fnB(); });
            static_assert(requires { self.Methods::fnB(); });
            static_assert(requires { self.Methods::B::fnB(); });
            static_assert(requires { self.Methods::AB::fnB(); });

            static_assert(requires { self.Resolve::getB(); });
            return self.getB().fnB();
        }
    };

    struct AandB : arc::Node
    {
        using Depends = arc::Depends<trait::A, trait::B>;
        using Traits = arc::Traits<trait::A, trait::B>;

        int impl(this auto const& self, trait::AB::fnA)
        {
            return self.getNode(trait::a).fnA();
        }

        int impl(this auto const& self, trait::AB::fnB)
        {
            return self.getNode(trait::b).fnB();
        }
    };
};

TEST_CASE("arc::JoinedTrait")
{
    arc::Graph<TestCluster, Root> g;

    CHECK(42 == g.asTrait(trait::a).fnA());
    CHECK(42 == g.asTrait(trait::aB).fnA());
    CHECK(84 == g.asTrait(trait::b).fnB());
    CHECK(84 == g.asTrait(trait::aB).fnB());

    CHECK(42 == g.ab.asTrait(trait::a).fnA());
    CHECK(42 == g.ab.asTrait(trait::aB).fnA());
    CHECK(84 == g.ab.asTrait(trait::b).fnB());
    CHECK(84 == g.ab.asTrait(trait::aB).fnB());

    CHECK(42 == g.aandb.asTrait(trait::a).fnA());
    CHECK(84 == g.aandb.asTrait(trait::b).fnB());
    static_assert(not g.aandb.hasTrait(trait::aB));
}

}
