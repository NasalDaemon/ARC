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
    g.a.asTrait(trait::trait).defer(i);
    CHECK(14 == i);
}

} // namespace arc::tests::repeater
