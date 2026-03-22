#include <doctest/doctest.h>
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <cstddef>
#include <optional>
#include <string>
#include <tuple>
#include <vector>
#endif

import arc.tests.datastore.cluster;
import arc.tests.datastore.types;
import arc;

/* arc-begin
module;
#include <map>
export module arc.tests.datastore.cluster;

import arc.tests.datastore.types;

namespace arc::tests::datastore {

cluster Cluster [R = Root]
{
    store = arc::node::DataStore<SharedData, std::map>
    r1 = R::Receiver1
    r2 = R::Receiver2

    using DataStore = arc::DataStore<SharedData>
    using DataListener = arc::DataListener<SharedData>

    [DataStore <-> DataListener]
    store <-> {r1, r2}
}

}

arc-end */

namespace arc::tests::datastore {

namespace trait {
    using DataStore = arc::DataStore<SharedData>;
    using DataListener = arc::DataListener<SharedData>;
}

struct Event
{
    std::string desc;
};

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

            void init(SharedData::Id const& id, SharedData& sharedData, auto constructor)
            {
                CHECK(id == sharedData.id);
                if (id < 4)
                {
                    PrivateData* ld = constructor(id + 10);
                    CHECK(ld->value == id + 10);
                }
            }

            #if ARC_COMPILER_CLANG
            // TODO: For some reason clang frontend crashes if this is not defined (but omitting with impl() syntax still works)
            bool skipEvent(auto&&...) { return false; }
            #endif

            bool onEvent(arc::Event<Event> event, EventItem<SharedData, PrivateData> item)
            {
                REQUIRE(event.isLatest());
                CHECK(item.id == item.sharedData.id);
                auto [pSharedData, pData] = getDataStore().get(item.id);
                CHECK(pSharedData == &item.sharedData);
                CHECK(pData == &item.privateData);
                if (event->desc == "test")
                    received.push_back(item.privateData);
                return true;
            }

            std::vector<PrivateData> received;
        };
    };

    struct Receiver2 : arc::Node::Impl<trait::DataListener*>::Uses<trait::DataStore>
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
            if (sharedData.id < 3)
            {
                PrivateData* ld = constructor(sharedData.id + 20);
                CHECK(ld->value == sharedData.id + 20);
            }
            else
            {
                skipped += 1;
            }
        }

        // Can omit impl for skipEvent since it is not required, and it will default to "do not skip"
        // bool impl(trait::DataListener::skipEvent, auto&&...) { return false; }

        void impl(this auto& self, trait::DataListener::onEvent, arc::Event<Event> event, EventItem<SharedData, PrivateData> item)
        {
            REQUIRE(event.isLatest());
            CHECK(item.id == item.sharedData.id);
            auto [pSharedData, pData] = self.getDataStore().get(item.id);
            CHECK(pSharedData == &item.sharedData);
            CHECK(pData == &item.privateData);
            PrivateData* pData2 = self.getDataStore().get(pSharedData);
            CHECK(pData2 == &item.privateData);
            if (event->desc == "test")
                self.received.push_back(item.privateData);
        }

        void testModify(this auto& self)
        {
            auto [pSharedData, pData] = self.getDataStore().modify(1, [](SharedData* sd, PrivateData* pd) {
                CHECK(sd != nullptr);
                CHECK(pd != nullptr);
                pd->value += 5;
                return Event{"test"};
            });
            CHECK(pSharedData != nullptr);
            CHECK(pData->value == 26);

            bool called = false;
            std::tie(pSharedData, pData) = self.getDataStore().modify(3, [&](SharedData const* sd, PrivateData* pd) {
                called = true;
                CHECK(sd != nullptr);
                CHECK(pd == nullptr);
                return noEvent;
            });
            CHECK(called);
            CHECK(pSharedData != nullptr);
            CHECK(pData == nullptr);

            called = false;
            pData = self.getDataStore().modify(pSharedData, [&](SharedData* sd, PrivateData* pd) {
                called = true;
                CHECK(sd != nullptr);
                CHECK(pd == nullptr);
                return std::optional<Event>{std::nullopt};
            });
            CHECK(called);
            CHECK(pSharedData != nullptr);
            CHECK(pData == nullptr);

            called = false;
            std::tie(pSharedData, pData) = self.getDataStore().modify(4, [&](SharedData const*, PrivateData*) {
                called = true;
                // no event returned
            });
            CHECK_FALSE(called);
            CHECK(pSharedData == nullptr);
            CHECK(pData == nullptr);
        }

        void testForEach(this auto& self)
        {
            std::size_t called = 0;
            self.getDataStore().forEach([&](int id, SharedData* sd, PrivateData* pd) {
                called++;
                REQUIRE(sd != nullptr);
                CHECK(sd->id == id);
                if (id == 3)
                {
                    CHECK(pd == nullptr);
                }
                else
                {
                    CHECK(pd != nullptr);
                    pd->value += 1;
                }
                return Event{"test"};
            });
            CHECK(called == 3);

            called = 0;
            self.getDataStore().forEach([&](int id, SharedData const* sd, PrivateData* pd) {
                called++;
                REQUIRE(sd != nullptr);
                CHECK(sd->id == id);
                if (id == 3)
                {
                    CHECK(pd == nullptr);
                }
                else
                {
                    CHECK(pd != nullptr);
                    pd->value += 1;
                }
                // no event returned
            });
            CHECK(called == 3);
        }

        std::vector<PrivateData> received;
        std::size_t skipped = 0;
    };
};

TEST_CASE("arc::DataStore")
{
    arc::Graph<cluster::Cluster, Root> graph;

    REQUIRE(graph.store->add(0, 0).second);
    REQUIRE(graph.store->add(1, 1).second);
    REQUIRE(graph.r2->skipped == 0);
    REQUIRE(graph.store->add(3, 3).second); // Receiver2 will not subscribe to this
    REQUIRE(graph.r2->skipped == 1);

    // Duplicate adds
    auto [data, added] = graph.store->add(1, 1);
    REQUIRE(graph.r2->skipped == 1);
    REQUIRE_FALSE(added);
    REQUIRE(data != nullptr);

    // Receiver2::init is called, but it skips again
    std::tie(data, added) = graph.store->add(3, 3);
    REQUIRE(graph.r2->skipped == 2);
    REQUIRE_FALSE(added);
    REQUIRE(data != nullptr);

    // Neither listener subscribes
    std::tie(data, added) = graph.store->add(4, 4);
    REQUIRE(graph.r2->skipped == 3);
    REQUIRE_FALSE(added);
    REQUIRE(data == nullptr);

    graph.store->notifyAll(Event{"test"});
    CHECK(graph.r1->received.size() == 3);
    CHECK(graph.r2->received.size() == 2);
    graph.store->notifyAll(Event{"not-test"}); // ignored by receivers
    CHECK(graph.r1->received.size() == 3);
    CHECK(graph.r2->received.size() == 2);

    CHECK(graph.r1->received.at(0).getValue() == 10);
    CHECK(graph.r1->received.at(1).getValue() == 11);
    CHECK(graph.r1->received.at(2).getValue() == 13);
    CHECK(graph.r2->received.at(0).getValue() == 20);
    CHECK(graph.r2->received.at(1).getValue() == 21);

    graph.r2->testModify();
    CHECK(graph.r1->received.size() == 4);
    CHECK(graph.r2->received.size() == 2);
    CHECK(graph.r1->received.at(3).getValue() == 11);

    graph.r2->testForEach();
    CHECK(graph.r1->received.size() == 7);
    CHECK(graph.r2->received.size() == 2);
    CHECK(graph.r1->received.at(4).getValue() == 10);
    CHECK(graph.r1->received.at(5).getValue() == 11);
    CHECK(graph.r1->received.at(6).getValue() == 13);
}

}
