#include <doctest/doctest.h>

#include "arc/macros.hpp"


import arc.tests.null_trait;
import arc;

namespace arc::tests::null_trait {

/* arc-embed-begin
export module arc.tests.null_trait;

namespace arc::tests::null_trait {

cluster Cluster [R = Root]
{
    node1 = R::Node
    node2 = R::Node

    [~] node1, node2
}

}

arc-embed-end */

struct Root
{
    struct Node : arc::Node
    {
        using Traits = arc::NoTraits<Node>;

        int doSomething(this auto const& self)
        {
            return self.getNode(arc::noTrait<Node>)->i;
        }

        int i;
    };
};

TEST_CASE("arc::NullTrait")
{
    arc::Graph<Cluster, Root> graph{
        .node1{ARC_EMPLACE({.i = 1})},
        .node2{ARC_EMPLACE({.i = 42})},
    };

    CHECK(graph.node1->doSomething() == 42);
    CHECK(graph.node2->doSomething() == 1);
}

}
