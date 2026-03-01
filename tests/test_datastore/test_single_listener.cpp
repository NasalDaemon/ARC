#include <doctest/doctest.h>
#include "arc/macros.hpp"

import arc;
import arc.tests.datastore.single;

/* arc-embed-begin
export module arc.tests.datastore.single;

namespace arc::tests::datastore::single {

cluster Cluster [R = Root] {
    store = arc::node::DataStore<typename R::SharedData>
    listener = R::Listener

    [arc::DataStore<typename R::SharedData> <-> arc::DataListener<typename R::SharedData>]
    store <-> listener
}

}
arc-embed-end */

namespace arc::tests::datastore::single {

struct Root
{
    struct SharedData
    {
        using Id = int;
        int id;
    };

    struct Listener
    {
        template<class Context>
        struct Node : arc::Node::Impl<arc::trait::DataListener<SharedData>>::Uses<arc::trait::DataStore<SharedData>>
        {
            struct Types
            {
                struct PrivateData
                {
                    int value = 42;
                };
            };
            void init(auto const&, auto&, auto constructor)
            {
                constructor();
            }

            #if ARC_COMPILER_CLANG
            bool skipEvent(auto&&...) { return false; }
            #endif

            void onEvent(auto event, auto item)
            {
                CHECK(event.isLatest());
                CHECK(event.get() == "test");
                CHECK(item.id == item.sharedData.id);
                CHECK(item.privateData.value == 42);
            }
        };
    };
};


TEST_CASE("arc::DataStore single listener") {
    arc::Graph<cluster::Cluster, Root> graph;
    graph.store->add(1, 1);
    auto [pShared, pPrivate] = graph.store->get(1);
    REQUIRE(pShared != nullptr);
    REQUIRE(pPrivate == nullptr);
    CHECK(pShared->id == 1);

    graph.store->notify("test", 1);
}

}
