#include <doctest/doctest.h>

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

import arc.tests.group;
import arc;

namespace arc::tests::group {

/* arc-embed-begin
export module arc.tests.group;

namespace arc::tests::group {

groups Class
{
    Low
    Mid
    High

    Low -> Mid, High
    Mid -> High
}

cluster Inner [Root]
{
    nodeHigh = Root::Default : WithGroup<Class::High>
    nodeMid = Root::Default : WithGroup<Class::Mid>
    nodeLow = Root::Default : WithGroup<Class::Low>
    node = Root::Default
    nodeHighLow = Root::Default : WithGroup<Class::High, Class::Low>
    nodeMidLow = Root::Default : WithGroup<Class::Mid, Class::Low>
    nodeHighMidLow = Root::Default : WithGroup<Class::High, Class::Mid, Class::Low>

    [~] nodeHigh, nodeMid, nodeLow, node, nodeHighLow, nodeMidLow, nodeHighMidLow
}

cluster Outer [Root]
{
    inner = Inner : WithGroup<Class::High>

    nodeHigh = Root::Default : WithGroup<Class::High>
    nodeMid = Root::Default : WithGroup<Class::Mid>
    nodeLow = Root::Default : WithGroup<Class::Low>
    node = Root::Default
    nodeHighLow = Root::Default : WithGroup<Class::High, Class::Low>
    nodeMidLow = Root::Default : WithGroup<Class::Mid, Class::Low>

    using HighTrait = arc::NoTrait<typename Root::High>
    using MidTrait = arc::NoTrait<typename Root::Mid>
    using LowTrait = arc::NoTrait<typename Root::Low>
    using HighLowTrait = arc::NoTrait<typename Root::HighLow>
    using MidLowTrait = arc::NoTrait<typename Root::MidLow>
    using DefaultTrait = arc::NoTrait<typename Root::Default>

    [HighTrait] inner
    [MidTrait] inner
    [LowTrait] inner
    [HighLowTrait] inner
    [MidLowTrait] inner
    [DefaultTrait] inner
}

}

arc-embed-end */

struct Root
{
    struct Default
    {
        template<class Context>
        struct Node : arc::Node
        {
            using Traits = arc::NoTraits<Node>;
            int i;
        };
    };

    using High = WithGroup<Default, Class::High>;
    using Mid = WithGroup<Default, Class::Mid>;
    using Low = WithGroup<Default, Class::Low>;
    using HighLow = WithGroup<Default, Class::High, Class::Low>;
    using MidLow = WithGroup<Default, Class::Mid, Class::Low>;
    using HighMidLow = WithGroup<Default, Class::High, Class::Mid, Class::Low>;
};

TEST_CASE("arc::WithGroup")
{
    arc::Graph<Inner, Root> graph{
        .nodeHigh{ARC_EMPLACE(.i = 1)},
        .nodeMid{ARC_EMPLACE(.i = 2)},
        .nodeLow{ARC_EMPLACE(.i = 3)},
        .node{ARC_EMPLACE(.i = 4)},
        .nodeHighLow{ARC_EMPLACE(.i = 5)},
        .nodeMidLow{ARC_EMPLACE(.i = 6)},
        .nodeHighMidLow{ARC_EMPLACE(.i = 7)},
    };

    // Default group can access all
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.node)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.nodeLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.nodeHighLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.nodeMidLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.nodeHighMidLow)>);
    CHECK(1 == graph.node->getNode(arc::noTrait<Root::High>)->i);
    CHECK(2 == graph.node->getNode(arc::noTrait<Root::Mid>)->i);
    CHECK(3 == graph.node->getNode(arc::noTrait<Root::Low>)->i);
    CHECK(5 == graph.node->getNode(arc::noTrait<Root::HighLow>)->i);
    CHECK(6 == graph.node->getNode(arc::noTrait<Root::MidLow>)->i);
    CHECK(7 == graph.node->getNode(arc::noTrait<Root::HighMidLow>)->i);

    // Low group can access low, mid, and high (Low->Mid,High)
    static_assert(not arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.node)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.nodeLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.nodeHighLow)>);  // HighLow inherits High
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.nodeMidLow)>);   // MidLow inherits Mid
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.nodeHighMidLow)>); // HighMidLow inherits High
    CHECK(1 == graph.nodeLow->getNode(arc::noTrait<Root::High>)->i);
    CHECK(2 == graph.nodeLow->getNode(arc::noTrait<Root::Mid>)->i);
    CHECK(5 == graph.nodeLow->getNode(arc::noTrait<Root::HighLow>)->i);
    CHECK(6 == graph.nodeLow->getNode(arc::noTrait<Root::MidLow>)->i);
    CHECK(7 == graph.nodeLow->getNode(arc::noTrait<Root::HighMidLow>)->i);

    // Mid group can access mid and high (Mid->High)
    static_assert(not arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.node)>);
    static_assert(not arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.nodeLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.nodeHighLow)>);  // HighLow inherits High
    static_assert(arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.nodeMidLow)>);   // MidLow inherits Mid
    static_assert(arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.nodeHighMidLow)>); // HighMidLow inherits High
    CHECK(1 == graph.nodeMid->getNode(arc::noTrait<Root::High>)->i);
    CHECK(5 == graph.nodeMid->getNode(arc::noTrait<Root::HighLow>)->i);
    CHECK(6 == graph.nodeMid->getNode(arc::noTrait<Root::MidLow>)->i);
    CHECK(7 == graph.nodeMid->getNode(arc::noTrait<Root::HighMidLow>)->i);

    // High group can only access high
    static_assert(not arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.node)>);
    static_assert(not arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.nodeLow)>);
    static_assert(not arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.nodeHighLow)>);
    static_assert(not arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.nodeMidLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.nodeHighMidLow)>);
    CHECK(5 == graph.nodeHigh->getNode(arc::noTrait<Root::HighLow>)->i);
    CHECK(7 == graph.nodeHigh->getNode(arc::noTrait<Root::HighMidLow>)->i);

    // HighLow group (JoinedGroup<High, Low>) is a composite group
    // It inherits from both High and Low, has union of permissions: Low->Mid,High
    // HighLow can access Low, Mid, and High nodes
    static_assert(not arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.node)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.nodeLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.nodeHighLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.nodeMidLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.nodeHighMidLow)>);
    CHECK(1 == graph.nodeHighLow->getNode(arc::noTrait<Root::High>)->i);
    CHECK(2 == graph.nodeHighLow->getNode(arc::noTrait<Root::Mid>)->i);
    CHECK(3 == graph.nodeHighLow->getNode(arc::noTrait<Root::Low>)->i);
    CHECK(6 == graph.nodeHighLow->getNode(arc::noTrait<Root::MidLow>)->i);
    CHECK(7 == graph.nodeHighLow->getNode(arc::noTrait<Root::HighMidLow>)->i);

    // MidLow group (JoinedGroup<Mid, Low>) is a composite group
    // It inherits from both Mid and Low, has union of permissions: Low->Mid,High and Mid->High
    // MidLow can access Low, Mid, and High nodes
    static_assert(not arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.node)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.nodeLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.nodeHighLow)>);  // HighLow inherits High
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.nodeMidLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.nodeHighMidLow)>);
    CHECK(1 == graph.nodeMidLow->getNode(arc::noTrait<Root::High>)->i);
    CHECK(2 == graph.nodeMidLow->getNode(arc::noTrait<Root::Mid>)->i);
    CHECK(3 == graph.nodeMidLow->getNode(arc::noTrait<Root::Low>)->i);
    CHECK(5 == graph.nodeMidLow->getNode(arc::noTrait<Root::HighLow>)->i);
    CHECK(7 == graph.nodeMidLow->getNode(arc::noTrait<Root::HighMidLow>)->i);

    // HighMidLow tests - comprehensive combination testing
    // JoinedGroup<High, Mid, Low> has union of all permissions: Low->Mid,High and Mid->High
    static_assert(not arc::PermissibleNode<decltype(graph.nodeHighMidLow), decltype(graph.node)>);  // Default not accessible
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighMidLow), decltype(graph.nodeLow)>);   // Low accessible (same group)
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighMidLow), decltype(graph.nodeMid)>);   // Mid accessible (Low->Mid, same group)
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighMidLow), decltype(graph.nodeHigh)>);  // High accessible (Low->High, Mid->High)
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighMidLow), decltype(graph.nodeHighLow)>); // HighLow accessible (inherits from High)
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighMidLow), decltype(graph.nodeMidLow)>);  // MidLow accessible (inherits from Mid)
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighMidLow), decltype(graph.nodeHighMidLow)>); // Self accessible
    CHECK(1 == graph.nodeHighMidLow->getNode(arc::noTrait<Root::High>)->i);
    CHECK(2 == graph.nodeHighMidLow->getNode(arc::noTrait<Root::Mid>)->i);
    CHECK(3 == graph.nodeHighMidLow->getNode(arc::noTrait<Root::Low>)->i);
    CHECK(5 == graph.nodeHighMidLow->getNode(arc::noTrait<Root::HighLow>)->i);
    CHECK(6 == graph.nodeHighMidLow->getNode(arc::noTrait<Root::MidLow>)->i);
}

TEST_CASE("arc::WithGroup nested")
{
    arc::Graph<Outer, Root> graph{
        .inner{
            .nodeHigh{ARC_EMPLACE(.i = 1)},
            .nodeMid{ARC_EMPLACE(.i = 2)},
            .nodeLow{ARC_EMPLACE(.i = 3)},
            .node{ARC_EMPLACE(.i = 4)},
            .nodeHighLow{ARC_EMPLACE(.i = 5)},
            .nodeMidLow{ARC_EMPLACE(.i = 6)},
            .nodeHighMidLow{ARC_EMPLACE(.i = 7)},
        },
        .nodeHigh{ARC_EMPLACE(.i = 8)},
        .nodeMid{ARC_EMPLACE(.i = 9)},
        .nodeLow{ARC_EMPLACE(.i = 10)},
        .node{ARC_EMPLACE(.i = 11)},
        .nodeHighLow{ARC_EMPLACE(.i = 12)},
        .nodeMidLow{ARC_EMPLACE(.i = 13)},
    };

    // Default group can access all
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.node)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.nodeLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.nodeHighLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.nodeMidLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.inner.node)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.inner.nodeLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.inner.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.inner.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.inner.nodeHighLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.inner.nodeMidLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.node), decltype(graph.inner.nodeHighMidLow)>);
    CHECK(1 == graph.node->getNode(arc::noTrait<Root::High>)->i);
    CHECK(2 == graph.node->getNode(arc::noTrait<Root::Mid>)->i);
    CHECK(3 == graph.node->getNode(arc::noTrait<Root::Low>)->i);

    // Low group can access low, mid, and high
    static_assert(not arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.node)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.nodeLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.nodeHighLow)>);  // HighLow inherits High
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.nodeMidLow)>);   // MidLow inherits Mid
    // Inner node inherits High group from Outer
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.inner.node)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.inner.nodeLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.inner.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.inner.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.inner.nodeHighLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.inner.nodeMidLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeLow), decltype(graph.inner.nodeHighMidLow)>);
    CHECK(1 == graph.nodeLow->getNode(arc::noTrait<Root::High>)->i);
    CHECK(2 == graph.nodeLow->getNode(arc::noTrait<Root::Mid>)->i);

    // Mid group can access mid and high
    static_assert(not arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.node)>);
    static_assert(not arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.nodeLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.nodeHighLow)>);  // HighLow inherits High
    static_assert(arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.nodeMidLow)>);   // MidLow inherits Mid
    // Inner nodes
    static_assert(arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.inner.node)>);
    static_assert(not arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.inner.nodeLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.inner.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.inner.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.inner.nodeHighLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.inner.nodeMidLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMid), decltype(graph.inner.nodeHighMidLow)>);
    CHECK(1 == graph.nodeMid->getNode(arc::noTrait<Root::High>)->i);

    // High group can only access high
    static_assert(not arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.node)>);
    static_assert(not arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.nodeLow)>);
    static_assert(not arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.nodeHighLow)>);  // HighLow inherits High
    static_assert(not arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.nodeMidLow)>); // MidLow doesn't inherit High
    // Inner node inherits High group from Outer
    static_assert(arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.inner.node)>);
    static_assert(not arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.inner.nodeLow)>);
    static_assert(not arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.inner.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.inner.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.inner.nodeHighLow)>);
    static_assert(not arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.inner.nodeMidLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHigh), decltype(graph.inner.nodeHighMidLow)>);

    // HighLow group tests
    static_assert(not arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.node)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.nodeLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.nodeHighLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.nodeMidLow)>);
    // Inner nodes
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.inner.node)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.inner.nodeLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.inner.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.inner.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.inner.nodeHighLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.inner.nodeMidLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeHighLow), decltype(graph.inner.nodeHighMidLow)>);
    CHECK(1 == graph.nodeHighLow->getNode(arc::noTrait<Root::High>)->i);
    CHECK(2 == graph.nodeHighLow->getNode(arc::noTrait<Root::Mid>)->i);
    CHECK(3 == graph.nodeHighLow->getNode(arc::noTrait<Root::Low>)->i);
    CHECK(6 == graph.nodeHighLow->getNode(arc::noTrait<Root::MidLow>)->i);

    // MidLow group tests
    static_assert(not arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.node)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.nodeLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.nodeHighLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.nodeMidLow)>);
    // Inner nodes
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.inner.node)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.inner.nodeLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.inner.nodeMid)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.inner.nodeHigh)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.inner.nodeHighLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.inner.nodeMidLow)>);
    static_assert(arc::PermissibleNode<decltype(graph.nodeMidLow), decltype(graph.inner.nodeHighMidLow)>);
    CHECK(1 == graph.nodeMidLow->getNode(arc::noTrait<Root::High>)->i);
    CHECK(2 == graph.nodeMidLow->getNode(arc::noTrait<Root::Mid>)->i);
    CHECK(3 == graph.nodeMidLow->getNode(arc::noTrait<Root::Low>)->i);
    CHECK(5 == graph.nodeMidLow->getNode(arc::noTrait<Root::HighLow>)->i);

}

}
