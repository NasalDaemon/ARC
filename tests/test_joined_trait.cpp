#include <doctest/doctest.h>
#include "arc/macros.hpp"

import arc.tests.joined_trait;
import arc;

/* arc-embed-begin

export module arc.tests.joined_trait;

namespace arc::tests::joined_trait {

trait A
{
    fnA() const -> int
}

trait B
{
    fnB() const -> int
}

trait AB = A + B

cluster TestCluster [Root]
{
    a = Root::ANode
    b = Root::BNode
    ab = Root::ABNode
    aandb = Root::AandB

    [A]
    .., ab --> a
    aandb --> ab
    [B]
    .., ab --> b
    aandb --> ab
    [AB]
    .. --> ab
}

}

arc-embed-end */

namespace arc::tests::joined_trait {

struct Root
{
    struct ANode : arc::Node
    {
        using Traits = arc::Traits<A>;

        int impl(A::fnA) const
        {
            return 42;
        }
    };

    struct BNode : arc::Node
    {
        using Traits = arc::Traits<B>;

        int impl(B::fnB) const
        {
            return 84;
        }
    };

    struct ABNode : arc::Node::Impl<AB*>::Uses<A, B>
    {
        int impl(this auto const& self, AB::fnA)
        {
            static_assert(requires { self.asA(); });
            static_assert(requires { self.AsTrait::asA(); });

            static_assert(requires { self.fnA(); });
            static_assert(requires { self.AB::Impl::fnA(); });
            static_assert(requires { self.Methods::AB::Impl::fnA(); });

            static_assert(requires { self.A::Impl::fnA(); });
            static_assert(requires { self.Methods::fnA(); });
            static_assert(requires { self.Methods::A::Impl::fnA(); });

            static_assert(requires { self.Resolve::getA(); });
            return self.getA().fnA();
        }

        int impl(this auto const& self, AB::fnB)
        {
            static_assert(requires { self.asB(); });
            static_assert(requires { self.AsTrait::asB(); });

            static_assert(requires { self.fnB(); });
            static_assert(requires { self.AB::Impl::fnB(); });
            static_assert(requires { self.Methods::AB::Impl::fnB(); });

            static_assert(requires { self.B::Impl::fnB(); });
            static_assert(requires { self.Methods::fnB(); });
            static_assert(requires { self.Methods::B::Impl::fnB(); });

            static_assert(requires { self.Resolve::getB(); });
            return self.getB().fnB();
        }
    };

    struct AandB : arc::Node
    {
        using Depends = arc::Depends<A, B>;
        using Traits = arc::Traits<A, B>;

        int impl(this auto const& self, AB::fnA)
        {
            return self.getNode(a).fnA();
        }

        int impl(this auto const& self, AB::fnB)
        {
            return self.getNode(b).fnB();
        }
    };
};

TEST_CASE("arc::JoinedTrait")
{
    arc::Graph<cluster::TestCluster, Root> g;

    CHECK(42 == g.asTrait(a).fnA());
    CHECK(42 == g.asTrait(aB).fnA());
    CHECK(84 == g.asTrait(b).fnB());
    CHECK(84 == g.asTrait(aB).fnB());

    CHECK(42 == g.ab.asTrait(a).fnA());
    CHECK(42 == g.ab.asTrait(aB).fnA());
    CHECK(84 == g.ab.asTrait(b).fnB());
    CHECK(84 == g.ab.asTrait(aB).fnB());

    CHECK(42 == g.aandb.asTrait(a).fnA());
    CHECK(84 == g.aandb.asTrait(b).fnB());
    static_assert(not g.aandb.hasTrait(aB));
}

}
