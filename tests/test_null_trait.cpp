#include <doctest/doctest.h>

#include "arc/macros.hpp"


import arc.tests.null_trait;
import arc;

namespace arc::tests::null_trait {

/* arc-begin
export module arc.tests.null_trait;

namespace arc::tests::null_trait {

cluster Cluster [R = Root]
{
    node1 = R::Node
    node2 = R::Node

    // Link nodes with NoTrait to each other
    // Only time that sink can have multiple nodes
    [~] node1, node2
}

}

arc-end */

struct Root
{
    struct Node : arc::Node
    {
        using Traits = arc::NoTraits;

        int doSomething(this auto const& self)
        {
            // Both syntaxes work and return the same address
            int const* a = &self.template getNode<Node>()->i;
            int const* b = &self.getNode(arc::noTrait<Node>)->i;
            CHECK(a == b);
            return *a;
        }

        int i;
    };
};

TEST_CASE("arc::NullTrait")
{
    arc::Graph<cluster::Cluster, Root> graph{
        .node1{ARC_EMPLACE({.i = 1})},
        .node2{ARC_EMPLACE({.i = 42})},
    };

    CHECK(graph.node1->doSomething() == 42);
    CHECK(graph.node2->doSomething() == 1);
}

} // namespace arc::tests::null_trait
