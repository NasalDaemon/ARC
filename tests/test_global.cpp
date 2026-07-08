#if !ARC_IMPORT_STD
#include <print>
#include <string_view>
#endif

import arc.tests.global;
import arc;

#include "arc/doctest.h"

namespace arc::tests::global {

/* arc-begin
export module arc.tests.global;

namespace arc::tests::global {

trait Log
{
    log(auto message) -> std::same_as<void> auto
}

cluster MainCluster [R = Root]
{
    node = R::Node
    cluster = cluster::InnerCluster

    [[@global]] @all ==> @global

    [~] .. --> node

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
    globalNode = R::Node

    [trait::Log]
    @parent --> logger

    [arc::NoTrait<typename R::Node>]
    @parent <-- logger
}

}

arc-end */

struct Root
{
    struct Node : arc::Node
    {
        using Traits = arc::NoTraits;
        using Depends = arc::Depends<arc::Global<trait::Log>>;

        int hello() { return value; }

        int value = 999;
    };
    struct Logger : arc::Node
    {
        using Traits = arc::Traits<trait::Log>;

        void impl(this auto& self, trait::Log::log, std::string_view message)
        {
            REQUIRE(self.getNode(arc::noTrait<Root::Node>)->hello() == 999);
            std::println("Log: {}", message);
            self.count++;
        }

        int count = 0;
    };
};

TEST_CASE("arc::Global")
{
    arc::GraphWithGlobal<cluster::MainCluster, cluster::GlobalCluster, Root> graph{
        .main{
            .node{},
            .cluster{},
        }
    };

    CHECK(graph.global.logger->count == 0);
    graph->node.getGlobal(trait::log).log("Hello, Global!");
    CHECK(graph.global.logger->count == 1);
    graph.asTrait(trait::log).log("Hello again, Global!");
    CHECK(graph.global.logger->count == 2);
    graph.global.globalNode.getGlobal(trait::log).log("Hello, Global!");
    CHECK(graph.global.logger->count == 3);
}

TEST_CASE("arc::Global hosted locally")
{
    arc::GraphWithGlobal<cluster::MainCluster, Root::Logger, Root> graph{
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

} // namespace arc::tests::global
