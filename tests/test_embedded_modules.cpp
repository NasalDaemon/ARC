#include <doctest/doctest.h>

import arc.tests.embedded;
import arc;

#if 0
// arc-embed-begin
export module arc.tests.embedded;

trait arc::tests::trait::Embedded
{
    get() const
}
// arc-embed-end

random code between two sections will be ignored

// arc-embed-begin
cluster arc::tests::EmbeddedCluster [R = Root]
{
    embedded = R::Embedded

    [trait::Embedded]
    .. --> embedded
}
// arc-embed-end
#endif

namespace arc::tests {

struct EmbeddedNode : arc::Node
{
    using Traits = arc::Traits<EmbeddedNode, trait::Embedded>;

    int impl(trait::Embedded::get) const { return 42; }
};

}

using namespace arc::tests;

TEST_CASE("embedded")
{
    struct Root
    {
        using Embedded = EmbeddedNode;
    };
    arc::Graph<EmbeddedCluster, Root> g{};

    CHECK(g.asTrait(trait::embedded).get() == 42);
}
