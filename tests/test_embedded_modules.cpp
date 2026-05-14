import arc.tests.embedded;
import arc;

#include "doctest.h"

/* arc-begin
export module arc.tests.embedded;

trait arc::tests::trait::Embedded
{
    get() const
}

cluster arc::tests::EmbeddedCluster [R = Root]
{
    embedded = R::Embedded

    [trait::Embedded]
    .. --> embedded
}
arc-end */

namespace arc::tests {

struct EmbeddedNode : arc::Node
{
    using Traits = arc::Traits<trait::Embedded>;

    int impl(trait::Embedded::get) const { return 42; }
};

} // namespace arc::tests

using namespace arc::tests;

TEST_CASE("embedded")
{
    struct Root
    {
        using Embedded = EmbeddedNode;
    };
    arc::Graph<cluster::EmbeddedCluster, Root> g{};

    CHECK(g.asTrait(trait::embedded).get() == 42);
}
