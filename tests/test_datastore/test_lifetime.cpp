#include "arc/doctest_prelude.h"
#include "arc/macros.hpp"

#include <version>

// GCC 14 lacks <flat_map>; GCC 15 ICEs (cp_emit_debug_info_for_using) instantiating it across modules
#if __cpp_lib_flat_map and not ARC_COMPILER_LT(GCC, 16)
#define ARC_TEST_FLAT_MAP 1
#else
#define ARC_TEST_FLAT_MAP 0
#endif

#if !ARC_IMPORT_STD
#include <algorithm>
#include <cstddef>
#if ARC_TEST_FLAT_MAP
#include <flat_map>
#else
#include <map>
#endif
#include <functional>
#include <optional>
#include <set>
#include <type_traits>
#include <utility>
#include <vector>
#endif

#if ARC_ASAN_ENABLED
#include <sanitizer/asan_interface.h>
#endif

import arc;
import arc.tests.datastore.lifetime;

#include "arc/doctest.h"

/* arc-begin
export module arc.tests.datastore.lifetime;

namespace arc::tests::datastore::lifetime {

cluster Cluster [R = Root]
{
    store = R::Store
    a = R::ListenerA
    b = R::ListenerB

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    store <-> {a, b}
}

}
arc-end */

namespace arc::tests::datastore::lifetime {

struct DataError {};
struct InitError {};

inline constexpr int throwingValue = -1;

struct Data
{
    using Id = int;

    Data(int value) : value(value)
    {
        if (value == throwingValue)
            throw DataError{};
    }

    int value;
};

struct Ping {};

template<int N>
struct Private
{
    explicit Private(int value) : value(value) { ++constructed; }
    Private(Private const&) = delete;
    Private& operator=(Private const&) = delete;
    ~Private() { ++destroyed; }

    int value;

    static inline int constructed = 0;
    static inline int destroyed = 0;
};

void resetCounts()
{
    Private<0>::constructed = Private<0>::destroyed = 0;
    Private<1>::constructed = Private<1>::destroyed = 0;
}

template<int N>
struct Listener
{
    template<class Context>
    struct Node : arc::Node::Impl<arc::trait::DataListener<Data>>::Uses<arc::trait::DataStore<Data>>
    {
        struct Types
        {
            using PrivateData = Private<N>;
        };

        using Item = EventItem<Data const, Private<N>>;

        void init(int id, auto const&, Data&, auto constructor)
        {
            ++inits;
            if (keepConstructor)
                keptConstructor = constructor;
            if (onInit)
                onInit(id);
            if (accepts(id))
                constructor(id);
        }

        void onEvent(arc::Event<Ping>, Item item)
        {
            events.push_back(item.id);
            if (onPing)
                onPing(item);
        }

        auto store() { return this->getDataStore(); }

        std::function<bool(int)> accepts = [](int) { return true; };
        std::function<void(int)> onInit;
        std::function<void(Item)> onPing;
        bool keepConstructor = false;
        std::function<Private<N>*(int)> keptConstructor;
        int inits = 0;
        std::vector<int> events;
    };
};

struct Root
{
    using ArcContractAssertHandler = arc::ThrowAssertHandler;
    using Data = lifetime::Data;
#if ARC_TEST_FLAT_MAP
    using Store = arc::node::DataStore<Data, std::flat_map>;
#else
    using Store = arc::node::DataStore<Data, std::map>;
#endif
    using ListenerA = Listener<0>;
    using ListenerB = Listener<1>;
};

struct IgnoreAssertRoot : Root
{
    using ArcContractAssertHandler = arc::IgnoreAssertHandler;
};

struct CopyError {};

template<int N>
struct Counted
{
    explicit Counted(int value) : value(value) { ++live; }
    Counted(Counted const& other) : value(other.value)
    {
        if (copiesBeforeThrow-- == 0)
            throw CopyError{};
        ++live;
    }
    Counted& operator=(Counted const&) = delete;
    ~Counted() { --live; }

    int value;

    static inline int live = 0;
    static inline int copiesBeforeThrow = -1;
};

template<int N>
struct CopyListener
{
    template<class Context>
    struct Node : arc::Node::Impl<arc::trait::DataListener<Data>>::Uses<arc::trait::DataStore<Data>>
    {
        struct Types
        {
            using PrivateData = Counted<N>;
        };

        void init(int id, auto const&, Data&, auto constructor) { constructor(id); }
        void onEvent(arc::Event<Ping>, auto) {}

        auto store() { return this->getDataStore(); }
    };
};

struct CopyRoot : Root
{
    using ListenerA = CopyListener<0>;
    using ListenerB = CopyListener<1>;
};

using TestGraph = arc::Graph<cluster::Cluster, Root>;
using CopyGraph = arc::Graph<cluster::Cluster, CopyRoot>;
using StoreRef = decltype(std::declval<TestGraph&>().store->get(0));
using ListenerRef = decltype(std::declval<TestGraph&>().a->store().get(0));
using StoreHandle = decltype(std::declval<StoreRef const&>().handle());
using StoreNode = std::remove_pointer_t<decltype(std::declval<TestGraph&>().store.operator->())>;

template<class R>
constexpr bool rejectsMisuse =
    not std::is_copy_constructible_v<R>
    and not std::is_move_constructible_v<R>
    and not std::is_copy_assignable_v<R>
    and not std::is_move_assignable_v<R>
    and not std::is_default_constructible_v<R>
    and not std::is_constructible_v<R, std::nullptr_t>
    and not std::is_constructible_v<R, StoreNode const&, std::nullptr_t>
    and not requires (R&& ref) { *std::move(ref); }
    and requires (R& ref) { *ref; ref->value; }
    and not std::is_convertible_v<R const&, bool>
    and std::is_constructible_v<bool, R const&>
    and not std::is_convertible_v<R const&, Data const*>
    and std::is_same_v<decltype(std::declval<R const&>().operator->()), Data const*>
    and std::is_same_v<decltype(*std::declval<R const&>()), Data const&>;

static_assert(not std::is_copy_constructible_v<StoreNode>);
static_assert(rejectsMisuse<StoreRef>);
static_assert(rejectsMisuse<ListenerRef>);

template<class G>
constexpr bool heapAllocatesRef = requires (G& graph) { new auto(graph.store->get(0)); };
template<class G>
constexpr bool heapAllocatesListenerRef = requires (G& graph) { new auto(graph.a->store().get(0)); };
template<class G>
constexpr bool heapAllocatesAdded = requires (G& graph) { new auto(graph.store->add(0, 0)); };
template<class G>
constexpr bool heapAllocatesRefArray = requires (G& graph) { new std::remove_cvref_t<decltype(graph.store->get(0))>[1]{graph.store->get(0)}; };

static_assert(not heapAllocatesRef<TestGraph>);
static_assert(not heapAllocatesListenerRef<TestGraph>);
static_assert(not heapAllocatesAdded<TestGraph>);
static_assert(not heapAllocatesRefArray<TestGraph>);

struct Unmovable
{
    Unmovable() = default;
    Unmovable(Unmovable const&) = delete;
    Unmovable& operator=(Unmovable const&) = delete;
};
struct Unallocatable : Unmovable
{
    static void* operator new(std::size_t) = delete;
};
static_assert(not arc::IsDataPin<Unmovable>);
static_assert(arc::IsDataPin<Unallocatable>);
static_assert(arc::IsDataPin<StoreRef> and arc::IsDataPin<ListenerRef>);
static_assert(std::is_same_v<decltype(std::declval<StoreRef const&>().privateData), void*>);
static_assert(std::is_same_v<decltype(std::declval<ListenerRef const&>().privateData), Private<0>*>);

using IgnoreAssertGraph = arc::Graph<cluster::Cluster, IgnoreAssertRoot>;
using IgnoreAssertHandle = decltype(std::declval<IgnoreAssertGraph&>().store->get(0).handle());
static_assert(sizeof(IgnoreAssertHandle) == 8);

SCENARIO("arc::DataStore Ref stays valid while a flat map relocates its entries")
{
    GIVEN("an entry and a Ref to it")
    {
        TestGraph graph;
        REQUIRE(graph.store->add(1, 11).inserted);
        auto ref = graph.store->get(1);
        Data const* const address = &*ref;

        WHEN("ten thousand other ids are added")
        {
            for (int id = 2; id <= 10'001; ++id)
                REQUIRE(graph.store->add(id, id).inserted);

            THEN("the Ref still reads the same entry")
            {
                CHECK(&*ref == address);
                CHECK(ref->value == 11);
                CHECK(ref.id() == 1);
                CHECK(graph.store->get(1).operator->() == address);
            }
        }
    }
}

SCENARIO("arc::DataStore release while a Ref is alive")
{
    GIVEN("an id only listener a subscribes to, pinned by a's Ref")
    {
        resetCounts();
        TestGraph graph;
        graph.b->accepts = [](int) { return false; };
        REQUIRE(graph.store->add(1, 11).inserted);

        WHEN("a releases the id")
        {
            {
                auto ref = graph.a->store().get(1);
                auto const handle = ref.handle();
                REQUIRE(ref.privateData != nullptr);
                Private<0> const& privateData = *ref.privateData;
                CHECK(graph.a->store().release(1));

                THEN("the id is gone at once but the Ref still reads the old data, its PrivateData unreachable by a new lookup")
                {
                    CHECK_FALSE(graph.store->get(1));
                    CHECK_FALSE(graph.store->resolve(handle));
                    CHECK(ref->value == 11);
                    CHECK(ref.privateData == &privateData);
                    CHECK(graph.a->store().resolve(ref.ref).privateData == nullptr);
                    CHECK(privateData.value == 1);
                    CHECK(Private<0>::destroyed == 0);
                    CHECK_FALSE(graph.a->store().release(1));
                }
            }

            THEN("the PrivateData is destroyed once the Ref leaves scope")
            {
                CHECK(Private<0>::destroyed == 1);
            }
        }

        WHEN("the id is added again while the old Ref pins it")
        {
            auto old = graph.store->get(1);
            REQUIRE(graph.a->store().release(1));
            REQUIRE(graph.store->add(1, 22).inserted);
            auto fresh = graph.store->get(1);

            THEN("the new entry takes a new slot and the old Ref keeps reading the old data")
            {
                CHECK(&*fresh != &*old);
                CHECK(old->value == 11);
                CHECK(fresh->value == 22);
                CHECK_FALSE(graph.store->resolve(old.handle()));
                CHECK(graph.store->resolve(fresh.handle()));
            }
        }

        WHEN("the id is subscribed to and added again while a's released PrivateData is still pinned")
        {
            bool subscribeAsserted = false;
            bool insertedWhilePinned = true;
            int initsWhilePinned = -1;
            {
                auto ref = graph.store->get(1);
                graph.b->accepts = [](int) { return true; };
                REQUIRE(graph.store->add(1, 11).inserted);
                REQUIRE(graph.a->store().release(1));
                try
                {
                    (void)graph.a->store().subscribe(1, 5);
                }
                catch (arc::ContractViolation const&)
                {
                    subscribeAsserted = true;
                }
                insertedWhilePinned = graph.store->add(1, 11).inserted;
                initsWhilePinned = graph.a->inits;
            }

            THEN("subscribe asserts, and add does not offer a the id")
            {
                CHECK(subscribeAsserted);
                CHECK_FALSE(insertedWhilePinned);
                CHECK(initsWhilePinned == 1);
                CHECK(Private<0>::constructed == 1);
            }

            AND_WHEN("the id is added once the pin is gone")
            {
                bool const inserted = graph.store->add(1, 11).inserted;

                THEN("a is offered it again")
                {
                    CHECK(inserted);
                    CHECK(graph.a->inits == initsWhilePinned + 1);
                    CHECK(Private<0>::constructed == 2);
                }
            }
        }
    }
}

SCENARIO("arc::DataStore listener releases itself while the store iterates")
{
    GIVEN("ids 1 to 3 only listener a subscribes to")
    {
        resetCounts();
        TestGraph graph;
        graph.b->accepts = [](int) { return false; };
        for (int id = 1; id <= 3; ++id)
            REQUIRE(graph.store->add(id, id * 10).inserted);

        WHEN("a releases the id inside onEvent during notify")
        {
            int destroyedInside = -1;
            int readAfterRelease = 0;
            graph.a->onPing = [&](auto item)
            {
                REQUIRE(graph.a->store().release(item.id));
                readAfterRelease = item.privateData.value + item.sharedData.value;
                destroyedInside = Private<0>::destroyed;
            };
            auto const after = graph.store->notify(Ping{}, 2);

            THEN("its data stays alive until the notification ends, and notify returns empty")
            {
                CHECK(readAfterRelease == 2 + 20);
                CHECK(destroyedInside == 0);
                CHECK(Private<0>::destroyed == 1);
                CHECK_FALSE(after);
                CHECK_FALSE(graph.store->get(2));
            }
        }

        WHEN("a releases each id inside onEvent during notifyAll")
        {
            graph.a->onPing = [&](auto item)
            {
                REQUIRE(graph.a->store().release(item.id));
                CHECK(item.privateData.value == item.id);
            };
            graph.store->notifyAll(Ping{});

            THEN("every entry is visited and destroyed after its visit")
            {
                CHECK(graph.a->events == std::vector{1, 2, 3});
                CHECK(Private<0>::destroyed == 3);
                CHECK_FALSE(graph.store->get(1));
            }
        }

        WHEN("a releases the id from a forEach callback")
        {
            std::vector<int> destroyedInside;
            graph.store->forEach([&](int id, Data const* data, void const*)
            {
                REQUIRE(graph.a->store().release(id));
                CHECK(data->value == id * 10);
                destroyedInside.push_back(Private<0>::destroyed);
            });

            THEN("each entry is destroyed only after the loop leaves it")
            {
                CHECK(destroyedInside == std::vector{0, 1, 2});
                CHECK(Private<0>::destroyed == 3);
            }
        }

        WHEN("the event returned by modify makes a release its last subscription")
        {
            graph.a->onPing = [&](auto item) { REQUIRE(graph.a->store().release(item.id)); };
            auto const after = graph.b->store().modify(1, [](Data* data, Private<1>* privateData)
            {
                CHECK(privateData == nullptr);
                data->value = 5;
                return Ping{};
            });

            THEN("modify returns empty")
            {
                CHECK(graph.a->events == std::vector{1});
                CHECK_FALSE(after);
            }
        }
    }
}

SCENARIO("arc::DataStore loops skip entries added during them")
{
    GIVEN("ids 1 to 3, with id 2 released so its slot is free")
    {
        TestGraph graph;
        for (int id = 1; id <= 3; ++id)
            REQUIRE(graph.store->add(id, id).inserted);
        REQUIRE(graph.a->store().release(2));
        REQUIRE(graph.b->store().release(2));
        REQUIRE_FALSE(graph.store->get(2));

        WHEN("forEach adds a new id for each id it visits")
        {
            std::vector<int> visited;
            graph.store->forEach([&](int id, Data const*, void const*)
            {
                visited.push_back(id);
                CHECK(graph.store->add(id + 100, id + 100).inserted);
            });

            THEN("only the ids present when it began are visited, though a new id reuses a slot it had yet to reach")
            {
                std::ranges::sort(visited);
                CHECK(visited == std::vector{1, 3});
                CHECK(graph.store->get(101)->value == 101);
                CHECK(graph.store->get(103)->value == 103);
            }
        }

        WHEN("notifyAll triggers an add from onEvent")
        {
            graph.a->onPing = [&](auto item)
            {
                if (item.id < 100)
                    (void)graph.store->add(item.id + 100, 0);
            };
            graph.store->notifyAll(Ping{});

            THEN("the added entries are not notified")
            {
                auto events = graph.a->events;
                std::ranges::sort(events);
                CHECK(events == std::vector{1, 3});
                CHECK(graph.store->get(101));
            }
        }
    }
}

SCENARIO("arc::DataStore add re-entered from init")
{
    GIVEN("listener a adds from its init")
    {
        TestGraph graph;

        WHEN("a adds the same id from init")
        {
            int nestedValue = 0;
            bool nestedInserted = true;
            graph.a->onInit = [&](int id)
            {
                auto [ref, privateData, inserted] = graph.store->add(id, 999);
                nestedInserted = inserted;
                nestedValue = ref->value;
            };
            auto const [ref, privateData, inserted] = graph.store->add(1, 11);

            THEN("the nested add returns the existing entry and runs no init, and b is still initialised")
            {
                CHECK_FALSE(nestedInserted);
                CHECK(nestedValue == 11);
                CHECK(inserted);
                CHECK(ref->value == 11);
                CHECK(graph.a->inits == 1);
                CHECK(graph.b->inits == 1);
                CHECK(graph.a->store().get(1).privateData != nullptr);
                CHECK(graph.b->store().get(1).privateData != nullptr);
            }
        }

        WHEN("no listener subscribes, but a takes a Ref from a nested add")
        {
            graph.a->accepts = graph.b->accepts = [](int) { return false; };
            int nestedValue = 0;
            graph.a->onInit = [&](int id) { nestedValue = graph.store->add(id, 999).ref->value; };
            auto const [ref, privateData, inserted] = graph.store->add(1, 11);

            THEN("the outer add stores nothing, and the nested Ref read the entry")
            {
                CHECK(nestedValue == 11);
                CHECK_FALSE(inserted);
                CHECK_FALSE(ref);
                CHECK_FALSE(graph.store->get(1));
            }
        }

        WHEN("a's init, offered an id only b subscribes to, releases b")
        {
            resetCounts();
            graph.a->accepts = [](int) { return false; };
            REQUIRE(graph.store->add(1, 11).inserted);
            graph.a->onInit = [&](int id) { REQUIRE(graph.b->store().release(id)); };
            auto const [ref, privateData, inserted] = graph.store->add(1, 11);

            THEN("b is not offered the id again while add pins its released PrivateData, and the entry is removed")
            {
                CHECK(graph.b->inits == 1);
                CHECK(graph.a->inits == 2);
                CHECK_FALSE(inserted);
                CHECK_FALSE(ref);
                CHECK_FALSE(graph.store->get(1));
                CHECK(Private<1>::constructed == 1);
                CHECK(Private<1>::destroyed == 1);
            }
        }

        WHEN("a adds a different id from init")
        {
            graph.a->onInit = [&](int id)
            {
                if (id == 1)
                    CHECK(graph.store->add(2, 22).inserted);
            };
            REQUIRE(graph.store->add(1, 11).inserted);

            THEN("both entries are stored and initialised by both listeners")
            {
                CHECK(graph.store->get(1)->value == 11);
                CHECK(graph.store->get(2)->value == 22);
                CHECK(graph.a->inits == 2);
                CHECK(graph.b->inits == 2);
            }
        }

        WHEN("a subscribes b from a's init")
        {
            int subscribedValue = 0;
            graph.a->onInit = [&](int id) { subscribedValue = graph.b->store().subscribe(id, 77).privateData->value; };
            REQUIRE(graph.store->add(1, 11).inserted);

            THEN("b is subscribed with those args and is not offered init")
            {
                CHECK(subscribedValue == 77);
                CHECK(graph.b->inits == 0);
                CHECK(graph.b->store().get(1).privateData->value == 77);
            }
        }

        WHEN("a subscribes itself from its own init")
        {
            graph.a->onInit = [&](int id) { (void)graph.a->store().subscribe(id, 5); };

            THEN("it asserts, and the add stores nothing")
            {
                CHECK_THROWS_AS((void)graph.store->add(1, 11), arc::ContractViolation);
                CHECK_FALSE(graph.store->get(1));
                CHECK(graph.b->inits == 0);
            }

            AND_WHEN("init no longer subscribes itself")
            {
                CHECK_THROWS_AS((void)graph.store->add(1, 11), arc::ContractViolation);
                graph.a->onInit = nullptr;

                THEN("the id can be added")
                {
                    CHECK(graph.store->add(1, 11).inserted);
                }
            }
        }
    }
}

SCENARIO("arc::DataStore subscribe")
{
    GIVEN("an id only listener a subscribes to")
    {
        resetCounts();
        TestGraph graph;
        graph.b->accepts = [](int) { return false; };
        REQUIRE(graph.store->add(1, 11).inserted);
        REQUIRE(graph.b->inits == 1);

        WHEN("b subscribes with args")
        {
            auto ref = graph.b->store().subscribe(1, 42);

            THEN("its PrivateData is constructed from them without init")
            {
                REQUIRE(ref.privateData != nullptr);
                CHECK(ref.privateData->value == 42);
                CHECK(graph.b->inits == 1);
            }

            AND_WHEN("b subscribes again")
            {
                auto again = graph.b->store().subscribe(1, 7);

                THEN("the existing PrivateData is returned")
                {
                    CHECK(again.privateData == ref.privateData);
                    CHECK(again.privateData->value == 42);
                    CHECK(Private<1>::constructed == 1);
                }
            }
        }

        WHEN("b keeps the constructor its init was passed and calls it after init returned")
        {
            graph.b->keepConstructor = true;
            REQUIRE(graph.store->add(2, 22).inserted);

            THEN("it asserts and constructs nothing")
            {
                REQUIRE(graph.b->keptConstructor);
                CHECK_THROWS_AS(graph.b->keptConstructor(5), arc::ContractViolation);
                CHECK(Private<1>::constructed == 0);
                CHECK(graph.b->store().get(2).privateData == nullptr);
            }
        }

        WHEN("b subscribes to a missing id")
        {
            auto ref = graph.b->store().subscribe(2, 42);

            THEN("it is empty")
            {
                CHECK_FALSE(ref);
                CHECK(Private<1>::constructed == 0);
            }
        }
    }
}

SCENARIO("arc::DataStore entries live while any listener is subscribed")
{
    GIVEN("an id both listeners subscribe to")
    {
        TestGraph graph;
        REQUIRE(graph.store->add(1, 11).inserted);

        WHEN("a releases it")
        {
            REQUIRE(graph.a->store().release(1));
            graph.store->notify(Ping{}, 1);

            THEN("the entry stays for b, and only b is notified")
            {
                CHECK(graph.store->get(1));
                CHECK(graph.a->events.empty());
                CHECK(graph.b->events == std::vector{1});
            }

            AND_WHEN("b releases it")
            {
                REQUIRE(graph.b->store().release(1));

                THEN("the entry is removed")
                {
                    CHECK_FALSE(graph.store->get(1));
                }
            }
        }

        WHEN("an id no listener subscribes to is added")
        {
            graph.a->accepts = graph.b->accepts = [](int) { return false; };
            auto const [ref, privateData, inserted] = graph.store->add(2, 22);

            THEN("nothing is stored")
            {
                CHECK_FALSE(inserted);
                CHECK_FALSE(ref);
                CHECK_FALSE(graph.store->get(2));
            }
        }

        WHEN("the entry is modified through a Ref")
        {
            auto ref = graph.store->get(1);
            graph.store->modify(ref, [](Data* data, void*)
            {
                data->value = 7;
                return Ping{};
            });

            THEN("the change is visible through the Ref and every listener is notified")
            {
                CHECK(ref->value == 7);
                CHECK(graph.a->events == std::vector{1});
                CHECK(graph.b->events == std::vector{1});
            }
        }

        WHEN("an empty Ref is dereferenced")
        {
            auto ref = graph.store->get(2);

            THEN("it asserts")
            {
                CHECK_THROWS_AS((void)*ref, arc::ContractViolation);
                CHECK_THROWS_AS((void)ref->value, arc::ContractViolation);
            }
        }
    }
}

SCENARIO("arc::DataStore handles")
{
    GIVEN("a handle to an entry")
    {
        TestGraph graph;
        REQUIRE(graph.store->add(1, 11).inserted);
        auto const handle = graph.store->get(1).handle();
        REQUIRE(graph.store->resolve(handle)->value == 11);

        WHEN("the entry is released")
        {
            REQUIRE(graph.a->store().release(1));
            REQUIRE(graph.b->store().release(1));

            THEN("the handle is stale")
            {
                CHECK_FALSE(graph.store->resolve(handle));
            }

            AND_WHEN("a different id reuses its slot")
            {
                REQUIRE(graph.store->add(2, 22).inserted);
                auto const reused = graph.store->get(2).handle();

                THEN("the old handle stays stale, and the new one resolves")
                {
                    CHECK(reused != handle);
                    CHECK_FALSE(graph.store->resolve(handle));
                    CHECK(graph.store->resolve(reused)->value == 22);
                }
            }
        }

        WHEN("a handle is taken from a Ref after its entry was released, and another id reuses the slot")
        {
            std::optional<StoreHandle> late;
            {
                auto ref = graph.store->get(1);
                REQUIRE(graph.a->store().release(1));
                REQUIRE(graph.b->store().release(1));
                late = ref.handle();
            }
            REQUIRE(graph.store->add(2, 22).inserted);

            THEN("the handle is empty and resolves to nothing")
            {
                CHECK(late->empty());
                CHECK_FALSE(graph.store->resolve(*late));
            }
        }

        WHEN("handles of several entries are kept in a set")
        {
            REQUIRE(graph.store->add(2, 22).inserted);
            std::set<StoreHandle> handles{handle, graph.store->get(2).handle(), graph.store->get(1).handle()};

            THEN("equal handles collapse, and none is empty")
            {
                CHECK(handles.size() == 2);
                CHECK(handles.contains(handle));
                CHECK(std::ranges::none_of(handles, &StoreHandle::empty));
            }
        }

        WHEN("another store instance is given a Ref from this one")
        {
            TestGraph other;
            REQUIRE(other.store->add(1, 11).inserted);
            auto ref = graph.store->get(1);

            THEN("it asserts")
            {
                CHECK_THROWS_AS(other.store->notify(Ping{}, ref), arc::ContractViolation);
            }
        }
    }
}

SCENARIO("arc::DataStore add is exception safe")
{
    GIVEN("a freed slot whose address is known")
    {
        TestGraph graph;
        REQUIRE(graph.store->add(9, 99).inserted);
        Data const* address = nullptr;
        {
            auto ref = graph.store->get(9);
            address = &*ref;
        }
        REQUIRE(graph.a->store().release(9));
        REQUIRE(graph.b->store().release(9));

        WHEN("SharedData's constructor throws")
        {
            bool threw = false;
            try
            {
                (void)graph.store->add(1, throwingValue);
            }
            catch (DataError const&)
            {
                threw = true;
            }

            THEN("nothing is stored, and the next add reuses the slot")
            {
                CHECK(threw);
                CHECK_FALSE(graph.store->get(1));
                #if ARC_ASAN_ENABLED
                CHECK(__asan_address_is_poisoned(address));
                #endif
                auto [ref, privateData, inserted] = graph.store->add(2, 22);
                CHECK(inserted);
                CHECK(&*ref == address);
                CHECK(ref->value == 22);
            }
        }

        WHEN("b's init throws after a subscribed")
        {
            graph.b->onInit = [](int) { throw InitError{}; };
            bool threw = false;
            try
            {
                (void)graph.store->add(1, 11);
            }
            catch (InitError const&)
            {
                threw = true;
            }

            THEN("the entry stays for a only")
            {
                CHECK(threw);
                REQUIRE(graph.store->get(1));
                CHECK(graph.a->store().get(1).privateData != nullptr);
                CHECK(graph.b->store().get(1).privateData == nullptr);
            }
        }
    }
}

SCENARIO("arc::DataStore with assertions disabled")
{
    GIVEN("a store whose root ignores assertions")
    {
        IgnoreAssertGraph graph;
        REQUIRE(graph.store->add(1, 11).inserted);

        WHEN("a listener takes a handle and releases the entry")
        {
            auto const handle = graph.a->store().get(1).handle();
            int const resolved = graph.b->store().resolve(handle)->value;
            REQUIRE(graph.a->store().release(1));
            REQUIRE(graph.b->store().release(1));

            THEN("the handle resolved while the entry lived, and is stale after")
            {
                CHECK(resolved == 11);
                CHECK_FALSE(graph.a->store().resolve(handle));
            }
        }
    }
}

SCENARIO("arc::DataStore copy")
{
    GIVEN("ids 1 to 3 both listeners subscribe to")
    {
        Counted<0>::copiesBeforeThrow = Counted<1>::copiesBeforeThrow = -1;
        CopyGraph graph;
        for (int id = 1; id <= 3; ++id)
            REQUIRE(graph.store->add(id, id * 10).inserted);
        REQUIRE(Counted<0>::live == 3);
        REQUIRE(Counted<1>::live == 3);

        WHEN("a PrivateData copy throws partway")
        {
            Counted<1>::copiesBeforeThrow = 1;
            bool threw = false;
            try
            {
                CopyGraph copy = graph;
            }
            catch (CopyError const&)
            {
                threw = true;
            }
            Counted<1>::copiesBeforeThrow = -1;

            THEN("nothing of the copy is left, and the original is untouched")
            {
                CHECK(threw);
                CHECK(Counted<0>::live == 3);
                CHECK(Counted<1>::live == 3);
                for (int id = 1; id <= 3; ++id)
                {
                    CHECK(graph.store->get(id)->value == id * 10);
                    CHECK(graph.a->store().get(id).privateData->value == id);
                    CHECK(graph.b->store().get(id).privateData->value == id);
                }
            }
        }

        WHEN("an entry released but still pinned in the original is copied")
        {
            auto const pinned = graph.store->get(2);
            REQUIRE(graph.a->store().release(2));
            REQUIRE(graph.b->store().release(2));
            CopyGraph copy = graph;

            THEN("its slot is free in the copy, which holds no PrivateData for it")
            {
                CHECK_FALSE(copy.store->get(2));
                CHECK(pinned->value == 20);
                CHECK(Counted<0>::live == 3 + 2);
                CHECK(Counted<1>::live == 3 + 2);
                REQUIRE(copy.store->add(4, 40).inserted);
                CHECK(copy.store->get(4)->value == 40);
                CHECK(copy.store->get(1)->value == 10);
            }
        }

        WHEN("a handle has gone stale by the time the store is copied")
        {
            auto const stale = graph.store->get(1).handle();
            REQUIRE(graph.a->store().release(1));
            REQUIRE(graph.b->store().release(1));
            REQUIRE(graph.store->add(5, 50).inserted);
            auto const fresh = graph.store->get(5).handle();
            CopyGraph copy = graph;

            THEN("it stays stale in the copy, and a live handle resolves there")
            {
                CHECK_FALSE(copy.store->resolve(stale));
                CHECK(copy.store->resolve(fresh)->value == 50);
            }
        }
    }
}

#if ARC_ASAN_ENABLED
SCENARIO("arc::DataStore poisons freed slots")
{
    GIVEN("an entry whose address is known")
    {
        TestGraph graph;
        REQUIRE(graph.store->add(1, 11).inserted);
        Data const* address = nullptr;
        {
            auto ref = graph.store->get(1);
            address = &*ref;
        }

        WHEN("it is released")
        {
            REQUIRE(graph.a->store().release(1));
            REQUIRE(graph.b->store().release(1));

            THEN("its slot is poisoned")
            {
                CHECK(__asan_address_is_poisoned(address));
            }

            AND_WHEN("another id reuses the slot")
            {
                REQUIRE(graph.store->add(2, 22).inserted);

                THEN("it is unpoisoned")
                {
                    CHECK(graph.store->get(2).operator->() == address);
                    CHECK_FALSE(__asan_address_is_poisoned(address));
                }
            }
        }
    }
}
#endif

} // namespace arc::tests::datastore::lifetime
