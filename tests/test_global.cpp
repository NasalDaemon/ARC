#include <doctest/doctest.h>

#if !ARC_IMPORT_STD
#include <print>
#include <string_view>
#endif

import arc.tests.global;
import arc;

namespace arc::tests::global {

/* arc-embed-begin
export module arc.tests.global;

namespace arc::tests::global {

trait trait::Log
{
    log(auto message) -> std::same_as<void> auto
}

cluster MainCluster [R = Root]
{
    node = R::Node
    cluster = InnerCluster

    [trait::Log]
    node --> ^
}

cluster InnerCluster [R = Root]
{
    node = R::Node

    [trait::Log] @global <-- @all
}

cluster GlobalCluster [R = Root]
{
    logger = R::Logger

    [trait::Log] logger
}

}

arc-embed-end */

struct Root
{
    struct Node : arc::Node
    {
        using Traits = arc::Traits<Node>;
        using Depends = arc::Depends<trait::Log, arc::Global<trait::Log>>;
    };
    struct Logger : arc::Node
    {
        using Traits = arc::Traits<Logger, trait::Log>;

        void impl(trait::Log::log, std::string_view message)
        {
            std::println("Log: {}", message);
            count++;
        }

        int count = 0;
    };
};

TEST_CASE("arc::Global")
{
    arc::Graph<GlobalCluster, Root> global;

    arc::GraphWithGlobal<MainCluster, GlobalCluster*, Root> graph{
        .global = &global,
        .main{
            .node{},
            .cluster{},
        }
    };
    CHECK(global.logger->count == 0);
    graph->node.getGlobal(trait::log).log("Hello, Global!");
    CHECK(global.logger->count == 1);
}

TEST_CASE("arc::Global hosted locally")
{
    arc::GraphWithGlobal<MainCluster, Root::Logger, Root> graph{
        .global{},
        .main{
            .node{},
            .cluster{},
        }
    };

    CHECK(graph.global->count == 0);

    // Explicit global resolution
    graph->node.getGlobal(trait::log).log("Hello, Global!");
    CHECK(graph.global->count == 1);

    // Redirected global resolution via *
    graph->node.getNode(trait::log).log("Hello, Global from InnerCluster!");
    CHECK(graph.global->count == 2);

    // Redirected global resolution via *
    graph->cluster.node.getNode(trait::log).log("Hello, Global from InnerCluster!");
    CHECK(graph.global->count == 3);
}

// TODO: Support arc::Box?

}
