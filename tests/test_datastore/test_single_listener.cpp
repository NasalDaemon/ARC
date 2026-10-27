#include "arc/doctest_prelude.h"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <string_view>
#endif

import arc;
import arc.tests.datastore.single;

#include "arc/doctest.h"

/* arc-begin
export module arc.tests.datastore.single;

namespace arc::tests::datastore::single {

cluster Cluster [R = Root] {
    store = R::SharedData : arc::node::DataStore
    listener = R::Listener
    viewer = R::Viewer

    [arc::DataStore<typename R::SharedData> <-> arc::DataListener<typename R::SharedData>]
    store <-> listener

    [arc::DataStore<typename R::SharedData>]
    viewer --> store
}

}
arc-end */

using namespace std::string_view_literals;

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
            void init(auto const&, auto const&, auto&, auto constructor)
            {
                constructor();
            }

            void onEvent(auto event, auto item)
            {
                CHECK(event.isLatest());
                CHECK(event.get() == "test");
                CHECK(item.id == item.sharedData.id);
                CHECK(item.privateData.value == 42);
                ++events;
            }

            int events = 0;
        };
    };

    struct Viewer
    {
        template<class Context>
        struct Node : arc::NodeUses<arc::trait::DataStore<SharedData>>
        {
            int read(int id) const { return getDataStore().get(id)->id; }

            void ping(int id) { getDataStore().notify("test"sv, getDataStore().get(id)); }
        };
    };
};


TEST_CASE("arc::DataStore single listener") {
    arc::Graph<cluster::Cluster, Root> graph;
    REQUIRE(graph.store->add(1, 1).inserted);
    auto ref = graph.store->get(1);
    REQUIRE(ref);
    CHECK(ref->id == 1);

    graph.store->notify("test"sv, 1);

    CHECK(graph.listener->events == 1);

    CHECK(graph.viewer->read(1) == 1);
    graph.viewer->ping(1);
    CHECK(graph.listener->events == 2);
}

} // namespace arc::tests::datastore::single
