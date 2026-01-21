#include <doctest/doctest.h>
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <bitset>
#include <cstddef>
#include <map>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#endif

import arc.tests.datastore.cluster;
import arc.tests.datastore.types;
import arc;

/* arc-embed-begin
module;
#include <map>
export module arc.tests.datastore.cluster;

import arc.tests.datastore.types;

namespace arc::tests::datastore {

cluster Cluster [R = Root]
{
    store = arc::DataStore<SharedData, std::map>
    r1 = R::Receiver1
    r2 = R::Receiver2

    using DataStore = arc::trait::DataStore<SharedData>
    using DataListener = arc::trait::DataListener<SharedData>

    [DataStore <-> DataListener]
    store <-> {r1, r2}
}

}

arc-embed-end */

namespace arc::tests::datastore {

namespace trait {
    using DataStore = arc::trait::DataStore<SharedData>;
    using DataListener = arc::trait::DataListener<SharedData>;
}

struct Root
{
    struct Receiver1
    {
        template<class Context>
        struct Node : arc::Node::Impl<trait::DataListener>::Uses<trait::DataStore>
        {
            struct Types
            {
                struct PrivateData
                {
                    PrivateData(int v = 1) : value(v) {}

                    int getValue() const { return value; }
                private:
                    friend Receiver1;
                    int value;
                };
            };

            using PrivateData = Types::PrivateData;

            void impl(trait::DataListener::init, SharedData::Id const& id, SharedData& sharedData, auto constructor)
            {
                CHECK(id == sharedData.id);
                PrivateData& ld = constructor(id + 10);
                CHECK(ld.value == id + 10);
            }

            void impl(trait::DataListener::onEvent, auto const& tag, SharedData::Id const& id, SharedData& sharedData, PrivateData& data)
            {
                CHECK(id == sharedData.id);
                auto [pSharedData, pData] = getDataStore().get(id);
                CHECK(pSharedData == &sharedData);
                CHECK(pData == &data);
                if (tag == "test")
                    received.push_back(data);
            }

            std::vector<PrivateData> received;
        };
    };

    struct Receiver2 : arc::Node::Impl<trait::DataListener>::Uses<trait::DataStore>
    {
        struct Types
        {
            struct PrivateData
            {
                PrivateData(int v = 2) : value(v) {}

                int getValue() const { return value; }

            private:
                friend Receiver2;
                int value;
            };
        };

        using PrivateData = Types::PrivateData;

        void impl(trait::DataListener::init, SharedData::Id const& id, SharedData& sharedData, auto constructor)
        {
            CHECK(id == sharedData.id);
            if (sharedData.id != 3)
            {
                PrivateData& ld = constructor(sharedData.id + 20);
                CHECK(ld.value == sharedData.id + 20);
            }
        }

        void impl(this auto& self, trait::DataListener::onEvent, auto const& tag, SharedData::Id const& id, SharedData& sharedData, PrivateData& data)
        {
            CHECK(id == sharedData.id);
            auto [pSharedData, pData] = self.getDataStore().get(id);
            CHECK(pSharedData == &sharedData);
            CHECK(pData == &data);
            if (tag == "test")
                self.received.push_back(data);
        }

        std::vector<PrivateData> received;
    };
};

TEST_CASE("arc::DataStore")
{
    arc::Graph<Cluster, Root> graph;

    REQUIRE(graph.store->add(0, 0));
    REQUIRE(graph.store->add(1, 1));
    REQUIRE(graph.store->add(3, 3)); // Receiver2 will not subscribe to this
    REQUIRE_FALSE(graph.store->add(1, 1)); // Duplicate

    graph.store->notifyAll("test");
    CHECK(graph.r1->received.size() == 3);
    CHECK(graph.r2->received.size() == 2);
    graph.store->notifyAll("not-test"); // ignored by receivers
    CHECK(graph.r1->received.size() == 3);
    CHECK(graph.r2->received.size() == 2);

    CHECK(graph.r1->received.at(0).getValue() == 10);
    CHECK(graph.r1->received.at(1).getValue() == 11);
    CHECK(graph.r1->received.at(2).getValue() == 13);
    CHECK(graph.r2->received.at(0).getValue() == 20);
    CHECK(graph.r2->received.at(1).getValue() == 21);
}

}
