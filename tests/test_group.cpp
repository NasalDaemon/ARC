#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

import arc.tests.group;
import arc;

#include "doctest.h"

namespace arc::tests::group {

/* arc-begin
export module arc.tests.group;

namespace arc::tests::group {

policy Class
{
    Low
    Mid
    High
    A = Other::A

    @nogroup -> Low,      Mid,      High
           A -> Low
                Low <=|-> Mid <=|-> High
                Low <======|------> High
}

policy Other
{
    A
    B

    A -> B
}

cluster Inner [Root]
{
    nodeHigh = Root::Default : InGroup<policy::Class::High>
    nodeMid = Root::Default : InGroup<policy::Class::Mid>
    nodeLow = Root::Default : InGroup<policy::Class::Low>
    node = Root::Default
    nodeHighLow = Root::Default : InGroup<policy::Class::High, policy::Class::Low>
    nodeMidLow = Root::Default : InGroup<policy::Class::Mid, policy::Class::Low>
    nodeHighMidLow = Root::Default : InGroup<policy::Class::High, policy::Class::Mid, policy::Class::Low>
    nodeOtherA = Root::Default : InGroup<policy::Other::A>

    [~] nodeHigh, nodeMid, nodeLow, node, nodeHighLow, nodeMidLow, nodeHighMidLow, nodeOtherA
}

cluster Outer [Root]
{
    inner = Inner : InGroup<policy::Class::High>

    nodeHigh = Root::Default : InGroup<policy::Class::High>
    nodeMid = Root::Default : InGroup<policy::Class::Mid>
    nodeLow = Root::Default : InGroup<policy::Class::Low>
    node = Root::Default
    nodeHighLow = Root::Default : InGroup<policy::Class::High, policy::Class::Low>
    nodeMidLow = Root::Default : InGroup<policy::Class::Mid, policy::Class::Low>

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

arc-end */

struct Root
{
    struct Default
    {
        template<class Context>
        struct Node : arc::Node
        {
            using Traits = arc::NoTraits;
            int i;

            bool writable() { return true; }
            bool writable() const { return false; }
        };
    };

    using High = InGroup<Default, policy::Class::High>;
    using Mid = InGroup<Default, policy::Class::Mid>;
    using Low = InGroup<Default, policy::Class::Low>;
    using HighLow = InGroup<Default, policy::Class::High, policy::Class::Low>;
    using MidLow = InGroup<Default, policy::Class::Mid, policy::Class::Low>;
    using HighMidLow = InGroup<Default, policy::Class::High, policy::Class::Mid, policy::Class::Low>;
    using OtherA = InGroup<Default, policy::Other::A>;
};

TEST_CASE("arc::InGroup")
{
    arc::Graph<cluster::Inner, Root> graph{
        .nodeHigh{ARC_EMPLACE({.i = 1})},
        .nodeMid{ARC_EMPLACE({.i = 2})},
        .nodeLow{ARC_EMPLACE({.i = 3})},
        .node{ARC_EMPLACE({.i = 4})},
        .nodeHighLow{ARC_EMPLACE({.i = 5})},
        .nodeMidLow{ARC_EMPLACE({.i = 6})},
        .nodeHighMidLow{ARC_EMPLACE({.i = 7})},
        .nodeOtherA{ARC_EMPLACE({.i = 8})},
    };

    // Default group can access all nodes in its own policy, but NOT nodes in policy::Other policy
    static_assert(arc::IsWritePermittedNode<decltype(graph.node), decltype(graph.node)>);
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.node), decltype(graph.nodeLow)>);
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.node), decltype(graph.nodeMid)>);
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.node), decltype(graph.nodeHigh)>);
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.node), decltype(graph.nodeHighLow)>);
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.node), decltype(graph.nodeMidLow)>);
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.node), decltype(graph.nodeHighMidLow)>);
    static_assert(not arc::IsReadPermittedNode<decltype(graph.node), decltype(graph.nodeOtherA)>);  // Cannot access policy::Other::A
    CHECK(1 == graph.node.getNode<Root::High>()->i);
    CHECK(2 == graph.node.getNode<Root::Mid>()->i);
    CHECK(3 == graph.node.getNode<Root::Low>()->i);
    CHECK(5 == graph.node.getNode<Root::HighLow>()->i);
    CHECK(6 == graph.node.getNode<Root::MidLow>()->i);
    CHECK(7 == graph.node.getNode<Root::HighMidLow>()->i);
    CHECK_FALSE(graph.node.getNode<Root::Low>()->writable());

    // Test alias: A = policy::Other::A in policy::Class policy, with rule A -> Low
    // This means policy::Other::A can connect TO Low nodes
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeOtherA), decltype(graph.nodeOtherA)>);     // Self access
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.nodeOtherA), decltype(graph.nodeLow)>);        // A -> Low (via alias)
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.nodeOtherA), decltype(graph.nodeHighLow)>);    // A -> Low
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.nodeOtherA), decltype(graph.nodeMidLow)>);     // A -> Low
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.nodeOtherA), decltype(graph.nodeHighMidLow)>); // A -> Low
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeOtherA), decltype(graph.nodeMid)>);    // No A -> Mid
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeOtherA), decltype(graph.nodeHigh)>);   // No A -> High
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeOtherA), decltype(graph.node)>);       // No A -> Default
    CHECK(3 == graph.nodeOtherA.getNode<Root::Low>()->i);
    CHECK_FALSE(graph.nodeOtherA.getNode<Root::Low>()->writable());

    // Low nodes cannot access policy::Other::A (connection is one-way: A -> Low, not Low -> A)
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.nodeOtherA)>);

    // Low group write access low, read access mid, and high (Low->Mid,High)
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.node)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeLow), decltype(graph.nodeLow)>);
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.nodeLow), decltype(graph.nodeMid)>);
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.nodeLow), decltype(graph.nodeHigh)>);
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.nodeLow), decltype(graph.nodeHighLow)>);  // HighLow inherits High
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.nodeLow), decltype(graph.nodeMidLow)>);   // MidLow inherits Mid
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.nodeLow), decltype(graph.nodeHighMidLow)>); // HighMidLow inherits High
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.nodeOtherA)>);  // Cannot access policy::Other::A
    CHECK(1 == graph.nodeLow.getNode<Root::High>()->i);
    CHECK(2 == graph.nodeLow.getNode<Root::Mid>()->i);
    CHECK(5 == graph.nodeLow.getNode<Root::HighLow>()->i);
    CHECK(6 == graph.nodeLow.getNode<Root::MidLow>()->i);
    CHECK(7 == graph.nodeLow.getNode<Root::HighMidLow>()->i);
    CHECK_FALSE(graph.nodeLow.getNode<Root::High>()->writable()); // High is read-only
    CHECK_FALSE(graph.nodeLow.getNode<Root::HighLow>()->writable()); // HighLow inherits High, is read-only from Low

    // Mid group can access mid and high (Mid->High)
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeMid), decltype(graph.node)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeMid), decltype(graph.nodeLow)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeMid), decltype(graph.nodeMid)>);
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.nodeMid), decltype(graph.nodeHigh)>);
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.nodeMid), decltype(graph.nodeHighLow)>);  // HighLow inherits Low
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeMid), decltype(graph.nodeMidLow)>);   // MidLow inherits Mid
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.nodeMid), decltype(graph.nodeHighMidLow)>); // HighMidLow inherits Mid, Low
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeMid), decltype(graph.nodeOtherA)>);  // Cannot access policy::Other::A
    CHECK(1 == graph.nodeMid.getNode<Root::High>()->i);
    CHECK(5 == graph.nodeMid.getNode<Root::HighLow>()->i);
    CHECK(6 == graph.nodeMid.getNode<Root::MidLow>()->i);
    CHECK(7 == graph.nodeMid.getNode<Root::HighMidLow>()->i);
    CHECK_FALSE(graph.nodeMid.getNode<Root::High>()->writable());
    CHECK(graph.nodeMid.getNode<Root::Low>()->writable());
    CHECK(graph.nodeMid.getNode<Root::MidLow>()->writable());

    // High group can only access high
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeHigh), decltype(graph.node)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.nodeLow)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.nodeMid)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.nodeHigh)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.nodeHighLow)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.nodeMidLow)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.nodeHighMidLow)>);
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeHigh), decltype(graph.nodeOtherA)>);  // Cannot access policy::Other::A
    CHECK(5 == graph.nodeHigh.getNode<Root::HighLow>()->i);
    CHECK(7 == graph.nodeHigh.getNode<Root::HighMidLow>()->i);
    CHECK(graph.nodeHigh.getNode<Root::Low>()->writable()); // Low is writable
    CHECK(graph.nodeHigh.getNode<Root::Mid>()->writable()); // Mid is writable
    CHECK(graph.nodeHigh.getNode<Root::MidLow>()->writable());

    // HighLow group (JoinedGroup<High, Low>) is a composite group
    // It inherits from both High and Low, has union of permissions: Low->Mid,High
    // HighLow can access Low, Mid, and High nodes
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeHighLow), decltype(graph.node)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHighLow), decltype(graph.nodeLow)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHighLow), decltype(graph.nodeMid)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHighLow), decltype(graph.nodeHigh)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHighLow), decltype(graph.nodeHighLow)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHighLow), decltype(graph.nodeMidLow)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHighLow), decltype(graph.nodeHighMidLow)>);
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeHighLow), decltype(graph.nodeOtherA)>);  // Cannot access policy::Other::A
    CHECK(1 == graph.nodeHighLow.getNode<Root::High>()->i);
    CHECK(2 == graph.nodeHighLow.getNode<Root::Mid>()->i);
    CHECK(3 == graph.nodeHighLow.getNode<Root::Low>()->i);
    CHECK(6 == graph.nodeHighLow.getNode<Root::MidLow>()->i);
    CHECK(7 == graph.nodeHighLow.getNode<Root::HighMidLow>()->i);
    CHECK(graph.nodeHighLow.getNode<Root::Mid>()->writable());
    CHECK(graph.nodeHighLow.getNode<Root::High>()->writable()); // HighLow inherits High, which is writable

    // MidLow group (JoinedGroup<Mid, Low>) is a composite group
    // It inherits from both Mid and Low, has union of permissions: Low->Mid,High and Mid->High
    // MidLow can access Low, Mid, and High nodes
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeMidLow), decltype(graph.node)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeMidLow), decltype(graph.nodeLow)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeMidLow), decltype(graph.nodeMid)>);
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.nodeMidLow), decltype(graph.nodeHigh)>);
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.nodeMidLow), decltype(graph.nodeHighLow)>);  // HighLow inherits Low
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeMidLow), decltype(graph.nodeMidLow)>);
    static_assert(arc::IsReadOnlyPermittedNode<decltype(graph.nodeMidLow), decltype(graph.nodeHighMidLow)>);
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeMidLow), decltype(graph.nodeOtherA)>);  // Cannot access policy::Other::A
    CHECK(1 == graph.nodeMidLow.getNode<Root::High>()->i);
    CHECK(2 == graph.nodeMidLow.getNode<Root::Mid>()->i);
    CHECK(3 == graph.nodeMidLow.getNode<Root::Low>()->i);
    CHECK(5 == graph.nodeMidLow.getNode<Root::HighLow>()->i);
    CHECK(7 == graph.nodeMidLow.getNode<Root::HighMidLow>()->i);
    CHECK(graph.nodeMidLow.getNode<Root::Mid>()->writable());
    CHECK(graph.nodeMidLow.getNode<Root::Low>()->writable());
    CHECK_FALSE(graph.nodeMidLow.getNode<Root::High>()->writable()); // High is read-only from Mid
    CHECK_FALSE(graph.nodeMidLow.getNode<Root::HighMidLow>()->writable()); // HighMidLow inherits High, is read-only from Mid/Low

    // HighMidLow tests - comprehensive combination testing
    // JoinedGroup<High, Mid, Low> has union of all permissions: Low->Mid,High and Mid->High
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeHighMidLow), decltype(graph.node)>);  // Default not accessible
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHighMidLow), decltype(graph.nodeLow)>);   // Low accessible (same group)
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHighMidLow), decltype(graph.nodeMid)>);   // Mid accessible (Low->Mid, same group)
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHighMidLow), decltype(graph.nodeHigh)>);  // High accessible (Low->High, Mid->High)
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHighMidLow), decltype(graph.nodeHighLow)>); // HighLow accessible (inherits from High)
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHighMidLow), decltype(graph.nodeMidLow)>);  // MidLow accessible (inherits from Mid)
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHighMidLow), decltype(graph.nodeHighMidLow)>); // Self accessible
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeHighMidLow), decltype(graph.nodeOtherA)>);  // Cannot access policy::Other::A
    CHECK(1 == graph.nodeHighMidLow.getNode<Root::High>()->i);
    CHECK(2 == graph.nodeHighMidLow.getNode<Root::Mid>()->i);
    CHECK(3 == graph.nodeHighMidLow.getNode<Root::Low>()->i);
    CHECK(5 == graph.nodeHighMidLow.getNode<Root::HighLow>()->i);
    CHECK(6 == graph.nodeHighMidLow.getNode<Root::MidLow>()->i);
    CHECK(graph.nodeHighMidLow.getNode<Root::High>()->writable());
    CHECK(graph.nodeHighMidLow.getNode<Root::Mid>()->writable());
    CHECK(graph.nodeHighMidLow.getNode<Root::Low>()->writable());
    CHECK(graph.nodeHighMidLow.getNode<Root::HighLow>()->writable());
    CHECK(graph.nodeHighMidLow.getNode<Root::MidLow>()->writable());
}

TEST_CASE("arc::InGroup nested")
{
    arc::Graph<cluster::Outer, Root> graph{
        .inner{
            .nodeHigh{ARC_EMPLACE({.i = 1})},
            .nodeMid{ARC_EMPLACE({.i = 2})},
            .nodeLow{ARC_EMPLACE({.i = 3})},
            .node{ARC_EMPLACE({.i = 4})},
            .nodeHighLow{ARC_EMPLACE({.i = 5})},
            .nodeMidLow{ARC_EMPLACE({.i = 6})},
            .nodeHighMidLow{ARC_EMPLACE({.i = 7})},
        },
        .nodeHigh{ARC_EMPLACE({.i = 8})},
        .nodeMid{ARC_EMPLACE({.i = 9})},
        .nodeLow{ARC_EMPLACE({.i = 10})},
        .node{ARC_EMPLACE({.i = 11})},
        .nodeHighLow{ARC_EMPLACE({.i = 12})},
        .nodeMidLow{ARC_EMPLACE({.i = 13})},
    };

    // Default group can access all
    static_assert(arc::IsReadPermittedNode<decltype(graph.node), decltype(graph.node)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.node), decltype(graph.nodeLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.node), decltype(graph.nodeMid)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.node), decltype(graph.nodeHigh)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.node), decltype(graph.nodeHighLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.node), decltype(graph.nodeMidLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.node), decltype(graph.inner.node)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.node), decltype(graph.inner.nodeLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.node), decltype(graph.inner.nodeMid)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.node), decltype(graph.inner.nodeHigh)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.node), decltype(graph.inner.nodeHighLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.node), decltype(graph.inner.nodeMidLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.node), decltype(graph.inner.nodeHighMidLow)>);
    CHECK(1 == graph.node.getNode<Root::High>()->i);
    CHECK(2 == graph.node.getNode<Root::Mid>()->i);
    CHECK(3 == graph.node.getNode<Root::Low>()->i);

    // Low group can access low, mid, and high
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.node)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.nodeLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.nodeMid)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.nodeHigh)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.nodeHighLow)>);  // HighLow inherits High
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.nodeMidLow)>);   // MidLow inherits Mid
    // Inner node inherits High group from Outer
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.inner.node)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.inner.nodeLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.inner.nodeMid)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.inner.nodeHigh)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.inner.nodeHighLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.inner.nodeMidLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeLow), decltype(graph.inner.nodeHighMidLow)>);
    CHECK(1 == graph.nodeLow.getNode<Root::High>()->i);
    CHECK(2 == graph.nodeLow.getNode<Root::Mid>()->i);

    // Mid group can access mid and high
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeMid), decltype(graph.node)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeMid), decltype(graph.nodeLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMid), decltype(graph.nodeMid)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMid), decltype(graph.nodeHigh)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMid), decltype(graph.nodeHighLow)>);  // HighLow inherits High
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMid), decltype(graph.nodeMidLow)>);   // MidLow inherits Mid
    // Inner nodes
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMid), decltype(graph.inner.node)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeMid), decltype(graph.inner.nodeLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMid), decltype(graph.inner.nodeMid)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMid), decltype(graph.inner.nodeHigh)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMid), decltype(graph.inner.nodeHighLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMid), decltype(graph.inner.nodeMidLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMid), decltype(graph.inner.nodeHighMidLow)>);
    CHECK(1 == graph.nodeMid.getNode<Root::High>()->i);

    // High group can only access high
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeHigh), decltype(graph.node)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.nodeLow)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.nodeMid)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeHigh), decltype(graph.nodeHigh)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeHigh), decltype(graph.nodeHighLow)>);  // HighLow inherits High
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.nodeMidLow)>); // MidLow doesn't inherit High
    // Inner node inherits High group from Outer
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.inner.node)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.inner.nodeLow)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.inner.nodeMid)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.inner.nodeHigh)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.inner.nodeHighLow)>);
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.inner.nodeMidLow)>);  // High can read Low (part of MidLow)
    static_assert(arc::IsWritePermittedNode<decltype(graph.nodeHigh), decltype(graph.inner.nodeHighMidLow)>);

    // HighLow group tests
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeHighLow), decltype(graph.node)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeHighLow), decltype(graph.nodeLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeHighLow), decltype(graph.nodeMid)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeHighLow), decltype(graph.nodeHigh)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeHighLow), decltype(graph.nodeHighLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeHighLow), decltype(graph.nodeMidLow)>);
    // Inner nodes
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeHighLow), decltype(graph.inner.node)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeHighLow), decltype(graph.inner.nodeLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeHighLow), decltype(graph.inner.nodeMid)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeHighLow), decltype(graph.inner.nodeHigh)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeHighLow), decltype(graph.inner.nodeHighLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeHighLow), decltype(graph.inner.nodeMidLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeHighLow), decltype(graph.inner.nodeHighMidLow)>);
    CHECK(1 == graph.nodeHighLow.getNode<Root::High>()->i);
    CHECK(2 == graph.nodeHighLow.getNode<Root::Mid>()->i);
    CHECK(3 == graph.nodeHighLow.getNode<Root::Low>()->i);
    CHECK(6 == graph.nodeHighLow.getNode<Root::MidLow>()->i);

    // MidLow group tests
    static_assert(not arc::IsReadPermittedNode<decltype(graph.nodeMidLow), decltype(graph.node)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMidLow), decltype(graph.nodeLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMidLow), decltype(graph.nodeMid)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMidLow), decltype(graph.nodeHigh)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMidLow), decltype(graph.nodeHighLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMidLow), decltype(graph.nodeMidLow)>);
    // Inner nodes
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMidLow), decltype(graph.inner.node)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMidLow), decltype(graph.inner.nodeLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMidLow), decltype(graph.inner.nodeMid)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMidLow), decltype(graph.inner.nodeHigh)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMidLow), decltype(graph.inner.nodeHighLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMidLow), decltype(graph.inner.nodeMidLow)>);
    static_assert(arc::IsReadPermittedNode<decltype(graph.nodeMidLow), decltype(graph.inner.nodeHighMidLow)>);
    CHECK(1 == graph.nodeMidLow.getNode<Root::High>()->i);
    CHECK(2 == graph.nodeMidLow.getNode<Root::Mid>()->i);
    CHECK(3 == graph.nodeMidLow.getNode<Root::Low>()->i);
    CHECK(5 == graph.nodeMidLow.getNode<Root::HighLow>()->i);

}

} // namespace arc::tests::group
