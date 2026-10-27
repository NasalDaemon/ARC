#include "arc/doctest_prelude.h"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <cstddef>
#include <map>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>
#endif

import arc;
import arc.tests.datastore.refs;

#include "arc/doctest.h"

/* arc-begin
export module arc.tests.datastore.refs;

namespace arc::tests::datastore::refs {

trait Schedule [Types]
{
    type Ref

    // begin: subscribe the scheduler to the entry the ref reads; false if it already was, or the entry is released
    begin(Types::Ref const& ref) -> bool
}

cluster Cluster [R = Root]
{
    store = R::Store
    owner = R::Owner
    scheduler = R::Scheduler
    viewer = R::Viewer
    arrivals = R::ArrivalLog

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    store <-> {owner, scheduler, arrivals}

    [arc::DataStore<typename R::Data>]
    viewer --> store

    [Schedule]
    owner, viewer --> scheduler
}

}
arc-end */

namespace arc::tests::datastore::refs {

struct Data
{
    using Id = int;
    int value;
};

struct Ping {};

std::size_t lookups = 0;

template<class Key, class Value>
struct CountingMap : std::map<Key, Value>
{
    auto find(Key const& key)
    {
        ++lookups;
        return std::map<Key, Value>::find(key);
    }
    auto find(Key const& key) const
    {
        ++lookups;
        return std::map<Key, Value>::find(key);
    }
};

using DataStore = arc::trait::DataStore<Data>;
using DataListener = arc::trait::DataListener<Data>;

struct Root
{
    using ArcContractAssertHandler = arc::ThrowAssertHandler;
    using Data = refs::Data;
    using Store = arc::node::DataStore<Data, CountingMap>;

    struct Owner
    {
        template<class Context>
        struct Node : arc::Node::Impl<DataListener>::Uses<DataStore, trait::Schedule>
        {
            struct Types
            {
                struct PrivateData
                {
                    int owned = 0;
                };
            };
            using PrivateData = Types::PrivateData;

            using Handle = arc::ResolveTypes<Node, DataStore>::Handle;

            void init(int, Handle const&, Data&, auto constructor) { constructor(); }
            void onEvent(arc::Event<Ping>, EventItem<Data const, PrivateData> item)
            {
                events.push_back(item.id);
                handles.push_back(item.handle);
            }

            auto store() { return this->getDataStore(); }

            bool schedule(int id)
            {
                auto ref = getDataStore().get(id);
                ++ref.privateData->owned;
                return getSchedule().begin(ref.ref);
            }

            int owned(int id) const
            {
                auto ref = getDataStore().get(id);
                static_assert(std::is_same_v<decltype(ref.privateData), PrivateData const*>);
                return ref.privateData->owned;
            }

            std::vector<int> events;
            std::vector<Handle> handles;
        };
    };

    struct Scheduler
    {
        template<class Context>
        struct Node : arc::Node::Impl<DataListener, trait::Schedule>::Uses<DataStore>
        {
            struct Types
            {
                struct PrivateData
                {
                    explicit PrivateData(int priority) : priority(priority) {}
                    int priority;
                };

                using Ref = arc::ResolveTypes<Node, DataStore>::Ref;
            };
            using PrivateData = Types::PrivateData;
            using Handle = arc::ResolveTypes<Node, DataStore>::Handle;
            using PrivateRef = arc::ResolveTypes<Node, DataStore>::PrivateRef;
            using ConstPrivateRef = arc::ResolveTypes<Node, DataStore>::ConstPrivateRef;

            void init(int, Handle const&, Data&, auto) {}
            void onEvent(arc::Event<Ping>, EventItem<Data const, PrivateData> item) { events.push_back(item.id); }

            auto store() { return this->getDataStore(); }

            bool begin(Types::Ref const& ref)
            {
                if (auto const resolved = getDataStore().resolve(ref); not resolved or resolved.privateData)
                    return false;
                if (not getDataStore().subscribe(ref, 1))
                    return false;
                queue.push_back(ref.handle());
                return true;
            }

            void raise(Types::Ref const& ref)
            {
                static_assert(std::is_same_v<arc::ContextOf<Node>, Context>);
                auto resolved = getDataStore().resolve(ref);
                static_assert(std::is_same_v<decltype(resolved), PrivateRef>);
                ++resolved.privateData->priority;
            }

            void raiseFirst()
            {
                auto resolved = getDataStore().resolve(queue.front());
                static_assert(std::is_same_v<decltype(resolved), PrivateRef>);
                ++resolved.privateData->priority;
            }

            int priority(Types::Ref const& ref) const
            {
                auto resolved = getDataStore().resolve(ref);
                static_assert(std::is_same_v<decltype(resolved), ConstPrivateRef>);
                return resolved.privateData->priority;
            }

            int firstPriority() const
            {
                auto resolved = getDataStore().resolve(queue.front());
                static_assert(std::is_same_v<decltype(resolved), ConstPrivateRef>);
                return resolved.privateData->priority;
            }

            std::vector<Handle> queue;
            std::vector<int> events;
        };
    };

    // Records each entry's handle as it arrives, oldest first
    struct ArrivalLog
    {
        template<class Context>
        struct Node : arc::Node::Impl<DataListener>::Uses<DataStore>
        {
            struct Types
            {
                using PrivateData = arc::NoPrivateData;
            };
            using Handle = arc::ResolveTypes<Node, DataStore>::Handle;

            struct Arrival
            {
                Handle handle;
                int id = 0;
            };

            // Not subscribing: it only keeps the order entries arrive in
            void init(int id, Handle const& handle, Data&, auto)
            {
                REQUIRE(getDataStore().resolve(handle));
                arrivals.push_back({handle, id});
                newest = Arrival{handle, id};
            }
            void onEvent(arc::Event<Ping>, EventItem<Data const, arc::NoPrivateData>) {}

            std::optional<int> oldestValue() const
            {
                auto const ref = getDataStore().resolve(arrivals.front().handle);
                return ref ? std::optional(ref->value) : std::nullopt;
            }

            Arrival popOldest()
            {
                Arrival const oldest = arrivals.front();
                arrivals.pop_front();
                return oldest;
            }

            arc::CircularBuffer<Arrival> arrivals;
            std::optional<Arrival> newest;
        };
    };

    struct Viewer
    {
        template<class Context>
        struct Node : arc::NodeUses<DataStore, trait::Schedule>
        {
            using Ref = arc::ResolveTypes<Node, DataStore>::Ref;
            using ConstPrivateRef = arc::ResolveTypes<Node, DataStore>::ConstPrivateRef;
            static_assert(std::is_same_v<Ref, typename arc::ResolveTypes<Node, trait::Schedule>::Ref>);
            static_assert(std::is_same_v<decltype(std::declval<ConstPrivateRef&>().privateData), void const*>);

            auto store() { return this->getDataStore(); }

            bool schedule(int id) { return getSchedule().begin(getDataStore().get(id).ref); }

            int value(int id) const
            {
                auto ref = getDataStore().get(id);
                static_assert(std::is_same_v<decltype(ref), ConstPrivateRef>);
                return ref->value;
            }
        };
    };
};

using TestGraph = arc::Graph<cluster::Cluster, Root>;
using OwnerRef = decltype(std::declval<TestGraph&>().owner->store().get(0));
using ViewerRef = decltype(std::declval<TestGraph&>().viewer->store().get(0));
using SchedulerRef = decltype(std::declval<TestGraph&>().scheduler->store().get(0));
using StoreRef = decltype(std::declval<OwnerRef&>().ref);

using ViewerStore = decltype(std::declval<TestGraph&>().viewer->store());
using OwnerStore = decltype(std::declval<TestGraph&>().owner->store());

template<class Store>
concept Subscribes = requires (Store store) { store.subscribe(0); store.release(0); };

static_assert(not Subscribes<ViewerStore>);
static_assert(Subscribes<OwnerStore>);

static_assert(std::is_same_v<StoreRef, decltype(std::declval<SchedulerRef&>().ref)>);
static_assert(std::is_same_v<StoreRef, decltype(std::declval<ViewerRef&>().ref)>);
static_assert(not std::is_same_v<OwnerRef, SchedulerRef>);
static_assert(std::is_same_v<decltype(std::declval<ViewerRef&>().privateData), void*>);
static_assert(not std::is_copy_constructible_v<StoreRef> and not std::is_move_constructible_v<StoreRef>);
static_assert(not std::is_copy_constructible_v<ViewerRef> and not std::is_move_constructible_v<ViewerRef>);

SCENARIO("arc::DataStore listener hands its entry to a sibling as a Ref")
{
    GIVEN("an entry only the owner subscribes to")
    {
        TestGraph graph;
        REQUIRE(graph.store->add(1, 11).inserted);
        lookups = 0;

        WHEN("the owner looks it up once and hands it to the scheduler")
        {
            bool const scheduled = graph.owner->schedule(1);

            THEN("the scheduler subscribes through the ref, and nothing else looks the entry up")
            {
                CHECK(scheduled);
                CHECK(lookups == 1);
                auto ref = graph.scheduler->store().get(1);
                REQUIRE(ref.privateData != nullptr);
                CHECK(ref.privateData->priority == 1);
                CHECK(graph.scheduler->queue == std::vector{ref.handle()});
                CHECK(graph.owner->owned(1) == 1);
            }

            AND_WHEN("it is handed over again")
            {
                bool const again = graph.owner->schedule(1);

                THEN("the scheduler resolves it into its own ref and sees it is already subscribed")
                {
                    CHECK_FALSE(again);
                    CHECK(graph.scheduler->queue.size() == 1);
                }
            }
        }

        WHEN("the scheduler resolves the owner's Ref")
        {
            auto owner = graph.owner->store().get(1);
            lookups = 0;
            auto resolved = graph.scheduler->store().resolve(owner.ref);

            THEN("it reads the same entry with its own PrivateData, without a lookup")
            {
                CHECK(lookups == 0);
                CHECK(&*resolved == &*owner);
                CHECK(resolved.id() == 1);
                CHECK(resolved.handle() == owner.handle());
                CHECK(resolved.privateData == nullptr);
                CHECK(graph.owner->store().resolve(owner.ref).privateData == owner.privateData);
            }

            THEN("a lookup unpacks into its Ref and the caller's PrivateData")
            {
                auto const [ownerRef, ownerData] = graph.owner->store().get(1);
                CHECK(&*ownerRef == &*owner);
                CHECK(ownerData == owner.privateData);

                auto const [viewerRef, viewerData] = graph.viewer->store().get(1);
                static_assert(std::is_same_v<decltype(viewerData), void* const>);
                CHECK(&*viewerRef == &*owner);
                CHECK(viewerData == nullptr);
            }

            THEN("a clone of its Ref outlives the lookup it was taken from and reads the same entry")
            {
                auto const clone = [&] { auto entry = graph.viewer->store().get(1); return entry.clone(); }();
                CHECK(&*clone == &*owner);
                CHECK(clone.handle() == owner.handle());
            }
        }
    }
}

SCENARIO("arc::DataStore listener resolves from const and non-const methods")
{
    GIVEN("an entry the scheduler subscribes to and holds the handle of")
    {
        TestGraph graph;
        REQUIRE(graph.store->add(1, 11).inserted);
        REQUIRE(graph.owner->schedule(1));
        auto const viewer = graph.viewer->store().get(1);

        WHEN("its non-const methods raise the priority through the Ref and the handle")
        {
            graph.scheduler->raise(viewer.ref);
            graph.scheduler->raiseFirst();

            THEN("its const methods read it through both")
            {
                CHECK(graph.scheduler->priority(viewer.ref) == 3);
                CHECK(graph.scheduler->firstPriority() == 3);
            }
        }
    }
}

SCENARIO("arc::DataStore listener records handles from init")
{
    GIVEN("two entries added in turn")
    {
        TestGraph graph;
        REQUIRE(graph.store->add(1, 11).inserted);
        REQUIRE(graph.store->add(2, 22).inserted);

        THEN("the listener holds their handles in arrival order, the newest also in a member")
        {
            REQUIRE(graph.arrivals->arrivals.size() == 2);
            CHECK(graph.arrivals->arrivals[0].id == 1);
            CHECK(graph.arrivals->arrivals[1].id == 2);
            CHECK(graph.arrivals->newest->handle == graph.viewer->store().get(2).handle());
        }

        WHEN("an entry is notified")
        {
            graph.viewer->store().notify(Ping{}, graph.viewer->store().get(2));

            THEN("its event carries the handle init was given")
            {
                CHECK(graph.owner->handles == std::vector{graph.arrivals->newest->handle});
            }
        }

        WHEN("the listener reads the oldest, pops it, and reads the next")
        {
            lookups = 0;
            std::optional<int> const oldest = graph.arrivals->oldestValue();
            int const popped = graph.arrivals->popOldest().id;
            std::optional<int> const next = graph.arrivals->oldestValue();

            THEN("it resolves the recorded handles without a lookup")
            {
                CHECK(lookups == 0);
                CHECK(oldest == 11);
                CHECK(popped == 1);
                CHECK(next == 22);
            }
        }

        WHEN("the oldest entry is released by its subscriber")
        {
            REQUIRE(graph.owner->store().release(1));

            THEN("its recorded handle resolves to nothing")
            {
                CHECK(graph.arrivals->oldestValue() == std::nullopt);
            }
        }
    }
}

SCENARIO("arc::DataStore non-listener reaches the store")
{
    GIVEN("an entry the owner subscribes to")
    {
        TestGraph graph;
        REQUIRE(graph.store->add(1, 11).inserted);

        WHEN("the viewer reads, modifies and notifies through its ref")
        {
            auto ref = graph.viewer->store().get(1);
            graph.viewer->store().modify(ref, [](Data* data, void*)
            {
                data->value = 12;
                return Ping{};
            });
            graph.viewer->store().notify(Ping{}, ref);

            THEN("the listeners are notified and the change is read by everyone")
            {
                CHECK(graph.owner->events == std::vector{1, 1});
                CHECK(ref->value == 12);
                CHECK(graph.viewer->value(1) == 12);
                CHECK(graph.viewer->store().resolve(ref.handle())->value == 12);
                auto const resolved = graph.viewer->store().resolve(ref.ref);
                CHECK(&*resolved == &*ref);
            }
        }

        WHEN("the viewer hands its ref to the scheduler")
        {
            bool const scheduled = graph.viewer->schedule(1);

            THEN("the scheduler subscribes to the entry")
            {
                CHECK(scheduled);
                CHECK(graph.scheduler->store().get(1).privateData != nullptr);
            }
        }

        WHEN("the viewer visits every entry")
        {
            std::vector<int> visited;
            graph.viewer->store().forEach([&](int id, Data const*, void const*) { visited.push_back(id); });

            THEN("it sees the entry")
            {
                CHECK(visited == std::vector{1});
            }
        }
    }
}

SCENARIO("arc::DataStore Ref outlives the release of its entry")
{
    GIVEN("a Ref to an entry only the owner subscribes to")
    {
        TestGraph graph;
        REQUIRE(graph.store->add(1, 11).inserted);
        auto const viewer = graph.viewer->store().get(1);
        auto const& storeRef = viewer.ref;

        WHEN("the owner releases the entry through its own ref")
        {
            {
                auto owner = graph.owner->store().resolve(storeRef);
                REQUIRE(graph.owner->store().release(owner));
            }

            THEN("the Ref still reads it, resolves without PrivateData, and cannot be subscribed through")
            {
                CHECK_FALSE(graph.store->get(1));
                CHECK(storeRef->value == 11);
                CHECK(storeRef.handle().empty());
                auto owner = graph.owner->store().resolve(storeRef);
                REQUIRE(owner);
                CHECK(owner.privateData == nullptr);
                CHECK_FALSE(graph.scheduler->begin(storeRef));
                CHECK(graph.scheduler->queue.empty());
            }
        }
    }
}

SCENARIO("arc::DataStore copied with its graph")
{
    GIVEN("an entry the owner and the scheduler subscribe to")
    {
        TestGraph graph;
        REQUIRE(graph.store->add(1, 11).inserted);
        REQUIRE(graph.owner->schedule(1));
        auto const handle = graph.viewer->store().get(1).handle();

        WHEN("the graph is copied")
        {
            TestGraph copy = graph;

            THEN("the copy holds the entry and each subscriber's PrivateData, and the handles its nodes hold resolve in it")
            {
                CHECK(copy.viewer->value(1) == 11);
                CHECK(copy.owner->owned(1) == 1);
                CHECK(copy.scheduler->store().get(1).privateData->priority == 1);
                REQUIRE(copy.scheduler->queue.size() == 1);
                CHECK(copy.scheduler->store().resolve(copy.scheduler->queue[0]).privateData->priority == 1);
                CHECK(copy.viewer->store().resolve(handle)->value == 11);
            }

            AND_WHEN("the copy releases the entry")
            {
                REQUIRE(copy.owner->store().release(1));
                REQUIRE(copy.scheduler->store().release(1));

                THEN("the original keeps it, and the copy's handles go stale")
                {
                    CHECK_FALSE(copy.store->get(1));
                    CHECK_FALSE(copy.viewer->store().resolve(handle));
                    CHECK(graph.viewer->value(1) == 11);
                    CHECK(graph.viewer->store().resolve(handle)->value == 11);
                }
            }

            AND_WHEN("the copy takes a new entry")
            {
                REQUIRE(copy.store->add(3, 33).inserted);
                auto const minted = copy.viewer->store().get(3).handle();

                THEN("its handle resolves in the copy")
                {
                    CHECK(copy.viewer->store().resolve(minted)->value == 33);
                }
            }
        }

        WHEN("the graph is moved")
        {
            TestGraph moved = std::move(graph);

            THEN("the handles its nodes hold resolve in the moved store")
            {
                REQUIRE(moved.scheduler->queue.size() == 1);
                CHECK(moved.scheduler->store().resolve(moved.scheduler->queue[0]).privateData->priority == 1);
                CHECK(moved.viewer->store().resolve(handle)->value == 11);
            }

            AND_WHEN("the moved-from store is used again")
            {
                REQUIRE(graph.store->add(2, 22).inserted);

                THEN("it starts empty")
                {
                    CHECK_FALSE(graph.store->get(1));
                    CHECK(graph.viewer->value(2) == 22);
                    CHECK(moved.viewer->value(1) == 11);
                }
            }
        }
    }
}

SCENARIO("arc::DataStore Ref from another store instance")
{
    GIVEN("a Ref from one graph")
    {
        TestGraph graph;
        TestGraph other;
        REQUIRE(graph.store->add(1, 11).inserted);
        REQUIRE(other.store->add(1, 11).inserted);
        auto const viewer = graph.viewer->store().get(1);
        auto const& storeRef = viewer.ref;

        THEN("resolving it through the other store asserts")
        {
            CHECK_THROWS_AS((void)other.viewer->store().resolve(storeRef), arc::ContractViolation);
            CHECK_THROWS_AS((void)other.scheduler->store().subscribe(storeRef, 1), arc::ContractViolation);
        }
    }
}

} // namespace arc::tests::datastore::refs
