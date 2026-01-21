#include <doctest/doctest.h>

#if !ARC_IMPORT_STD
#include <cstdio>
#endif

import arc.tests.repeater.cluster;
import arc;

namespace arc::tests::repeater {

TEST_CASE("TestRepeater")
{
    arc::Graph<Cluster> g;
    int i = 12;
    g.a.asTrait(trait::source).defer(i);
    CHECK(14 == i);
    CHECK(1 == g.b->functionCalled);
    g.a.getNode(trait::target, key::repeaterNode<TargetNode>).function(i);
    CHECK(15 == i);
    CHECK(2 == g.b->functionCalled);
    g.a.getNode(trait::target, key::repeaterIndex<0>).function(i);
    CHECK(16 == i);
    CHECK(3 == g.b->functionCalled);
}

} // namespace arc::tests::repeater
