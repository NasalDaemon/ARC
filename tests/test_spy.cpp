#include <doctest/doctest.h>

#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

import arc.tests.spy;
import arc;

namespace arc::tests::spy {

/* arc-embed-begin
export module arc.tests.spy;

namespace arc::tests::spy {

cluster Global [Root]
{
    spy1 = Root::Spy
    spy2 = Root::Spy

    // Test chain of spies
    [arc::trait::Spy]
    .. --> spy1 --> spy2
}

cluster Cluster [Root]
{
    node1 = Root::Node
    node2 = Root::Node

    [trait::Something]
    node1 <--> node2

    [trait::SomethingElse]
    node1 <--> node2
}

trait trait::Something
{
    doSomething(int x, int y) -> std::same_as<int> auto
}

trait trait::SomethingElse
{
    doSomethingElse(int x, int y) -> std::same_as<int> auto
}

}

arc-embed-end */

struct Root
{
    struct Node : arc::Node
    {
        using Traits = arc::Traits<Node, trait::Something, trait::SomethingElse>;

        int impl(trait::Something::doSomething, int x, int y) const
        {
            return x + y + i;
        }

        int impl(trait::SomethingElse::doSomethingElse, int x, int y) const
        {
            return x * y + i;
        }

        int i;
    };

    struct Spy : arc::Node
    {
        struct Something;

        using Traits = arc::Traits<Spy
            , arc::trait::SpyOnly<trait::Something>(Something)
            , arc::trait::Spy
        >;

        template<class Self, class Method>
        auto impl(this Self& self, arc::trait::Spy::intercept, Method method, auto f, int x, int y) -> decltype(auto)
        {
            static_assert(std::is_same_v<Method, trait::SomethingElse::doSomethingElse>);
            // Test chain of spies
            if constexpr (arc::CanResolve<Self, arc::trait::SpyOnly<Method>>)
            {
                return self.getNode(arc::trait::spyOnly<Method>).intercept(method, f, x, y);
            }
            else
            {
                ++self.doSomethingElseCount;
                if (x == y)
                    x *= 2;

                return f(x, y);
            }
        }

        int doSomethingCount = 0;
        int doSomethingElseCount = 0;
    };
};

struct Root::Spy::Something : Root::Spy
{
    auto impl(arc::trait::Spy::intercept, trait::Something::doSomething, auto f, int x, int y) -> decltype(auto)
    {
        ++doSomethingCount;
        if (x == y)
            x *= 2;

        return f(x, y);
    }
};

TEST_CASE("arc::trait::Spy")
{
    arc::GraphWithGlobal<Cluster, Global, Root> graph{
        .main{
            .node1{ARC_EMPLACE(.i = 1)},
            .node2{ARC_EMPLACE(.i = 42)},
        }
    };

    REQUIRE(graph.global->spy1->doSomethingCount == 0);
    REQUIRE(graph.global->spy2->doSomethingCount == 0);
    CHECK(graph->node1->getNode(trait::something).doSomething(41, 42) == 125);
    CHECK(graph->node2->getNode(trait::something).doSomething(41, 42) == 84);
    CHECK(graph->node1->getNode(trait::something).doSomething(42, 42) == 168);
    CHECK(graph->node2->getNode(trait::something).doSomething(42, 42) == 127);
    CHECK(graph.global->spy1->doSomethingCount == 4);
    CHECK(graph.global->spy2->doSomethingCount == 0);

    REQUIRE(graph.global->spy1->doSomethingElseCount == 0);
    REQUIRE(graph.global->spy2->doSomethingElseCount == 0);
    CHECK(graph->node1->getNode(trait::somethingElse).doSomethingElse(41, 42) == 1764);
    CHECK(graph->node2->getNode(trait::somethingElse).doSomethingElse(41, 42) == 1723);
    CHECK(graph->node1->getNode(trait::somethingElse).doSomethingElse(42, 42) == 3570);
    CHECK(graph->node2->getNode(trait::somethingElse).doSomethingElse(42, 42) == 3529);
    CHECK(graph.global->spy1->doSomethingElseCount == 0);
    CHECK(graph.global->spy2->doSomethingElseCount == 4);
}

}
