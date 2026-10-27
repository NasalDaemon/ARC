#include "arc/doctest_prelude.h"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <cstdint>
#include <type_traits>
#include <utility>
#endif

import arc;
import arc.tests.datastore.topology;

#include "arc/doctest.h"

/* arc-begin
export module arc.tests.datastore.topology;

namespace arc::tests::datastore::topology {

trait Inspect
{
    privateValue(int id) -> int
    events() const -> int
    touch(int id)
    releaseOwn(int id) -> bool
}

trait Peek
{
    value(int id) -> int
    ping(int id)
}

cluster Sub [R = Root]
{
    inner = R::Probe
    side = R::Side

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    .. <-> inner

    [arc::DataStore<typename R::Data>]
    side --> ..

    [Inspect]
    .. --> inner

    [Peek]
    .. --> side
}

cluster Deep [R = Root]
{
    sub = cluster::Sub

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    .. <-> sub

    [Inspect]
    .. --> sub

    [Peek]
    .. --> sub
}

cluster SideOnly [R = Root]
{
    side = R::Side

    [arc::DataStore<typename R::Data>]
    side --> ..

    [Peek]
    .. --> side
}

cluster StoreBox [R = Root]
{
    store = R::Store

    [arc::DataStore<typename R::Data>]
    .. --> store

    [arc::DataListener<typename R::Data>]
    store --> ..
}

cluster InSub [R = Root]
{
    store = R::Store
    sub = cluster::Sub

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    store <-> sub
}

cluster InDeep [R = Root]
{
    store = R::Store
    deep = cluster::Deep

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    store <-> deep
}

cluster InUnionSub [R = Root]
{
    store = R::Store
    sub = cluster::Sub : arc::Union

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    store <-> sub
}

cluster InLazyLazy [R = Root]
{
    store = R::Store
    probe = R::Probe : arc::Lazy : arc::Lazy

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    store <-> probe
}

cluster InUnion [R = Root]
{
    store = R::Store
    probe = arc::Union<typename R::Probe, typename R::ProbeB>

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    store <-> probe
}

cluster WithLazySideOnly [R = Root]
{
    store = R::Store
    probe = R::Probe
    sub = cluster::SideOnly : arc::Lazy

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    store <-> probe

    [arc::DataStore<typename R::Data>]
    sub --> store
}

cluster OneOfTwoWrapped [R = Root]
{
    store = R::Store
    plain = R::Probe
    wrapped = R::ProbeB : arc::Lazy

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    store <-> {plain, wrapped}
}

cluster OutsideStoreBox [R = Root]
{
    box = cluster::StoreBox
    probe = R::Probe

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    box <-> probe
}

}
arc-end */

namespace arc::tests::datastore::topology {

struct Data
{
    using Id = int;
    int value;
};

struct Ping {};

using DataStore = arc::trait::DataStore<Data>;
using DataListener = arc::trait::DataListener<Data>;

// One type for every probe, as arc::Union requires of its options
struct ProbeData
{
    int value;
};

template<int Value>
struct ProbeT
{
    template<class Context>
    struct Node : arc::Node::Impl<DataListener, trait::Inspect>::Uses<DataStore>
    {
        struct Types
        {
            using PrivateData = ProbeData;
        };

        void init(int, auto const&, Data&, auto constructor) { constructor(Value); }
        void onEvent(arc::Event<Ping>, auto) { ++received; }

        int privateValue(int id) { return getDataStore().get(id).privateData->value; }
        int events() const { return received; }
        void touch(int id) { getDataStore().modify(id, [](Data*, auto*) { return Ping{}; }); }
        bool releaseOwn(int id) { return getDataStore().release(id); }

        int received = 0;
    };
};

struct Side
{
    template<class Context>
    struct Node : arc::Node::Impl<trait::Peek>::Uses<DataStore>
    {
        int value(int id)
        {
            static_assert(std::is_same_v<decltype(std::declval<typename arc::ResolveTypes<Node, DataStore>::PrivateRef&>().privateData), void*>);
            return getDataStore().get(id)->value;
        }
        void ping(int id) { getDataStore().notify(Ping{}, getDataStore().get(id)); }
    };
};

struct Root
{
    using ArcContractAssertHandler = arc::ThrowAssertHandler;
    using Data = topology::Data;
    using Store = arc::node::DataStore<Data>;
    using Probe = ProbeT<7>;
    using ProbeB = ProbeT<8>;
    using Side = topology::Side;
};

template<class H>
concept ImplicitFromRaw = requires (void (*take)(H)) { take({1u, 2u}); };

template<class Cluster>
using StoreHandle = decltype(std::declval<arc::Graph<Cluster, Root>&>().store->add(1, 1).handle());

// A Handle's type depends on SharedData only
static_assert(std::is_same_v<StoreHandle<cluster::InSub>, StoreHandle<cluster::InDeep>>);
static_assert(std::is_same_v<StoreHandle<cluster::InSub>, arc::DataHandle<Data>>);
// Any implementation of the trait makes them from the raw slot and generation
static_assert(std::is_constructible_v<arc::DataHandle<Data>, std::uint32_t, std::uint32_t>);
static_assert(not ImplicitFromRaw<arc::DataHandle<Data>>);
static_assert(not std::is_default_constructible_v<arc::DataHandle<Data>>);
static_assert(std::is_trivially_copyable_v<arc::DataHandle<Data>> and sizeof(arc::DataHandle<Data>) == 8);

// The listener reads its own PrivateData, is not sent the event of its own modify, is sent others', and releases
void checkListener(auto& store, auto listener, int expected)
{
    REQUIRE(store->add(1, 11).inserted);
    CHECK(listener.privateValue(1) == expected);
    listener.touch(1);
    CHECK(listener.events() == 0);
    store->notify(Ping{}, 1);
    CHECK(listener.events() == 1);
    CHECK(listener.releaseOwn(1));
    CHECK_FALSE(store->get(1));
}

TEST_CASE("arc::DataStore listener in a subcluster, beside a non-listener")
{
    arc::Graph<cluster::InSub, Root> graph;
    REQUIRE(graph.store->add(2, 22).inserted);
    CHECK(graph.sub.asTrait(trait::peek).value(2) == 22);
    checkListener(graph.store, graph.sub.asTrait(trait::inspect), 7);
}

TEST_CASE("arc::DataStore listener in nested subclusters")
{
    arc::Graph<cluster::InDeep, Root> graph;
    checkListener(graph.store, graph.deep.asTrait(trait::inspect), 7);
}

TEST_CASE("arc::DataStore listener in a subcluster inside arc::Union, beside a non-listener")
{
    arc::Graph<cluster::InUnionSub, Root> graph{.sub{std::in_place_index<0>}};
    REQUIRE(graph.store->add(2, 22).inserted);
    CHECK(graph.sub->asTrait(trait::peek).value(2) == 22);
    checkListener(graph.store, graph.sub->asTrait(trait::inspect), 7);
}

TEST_CASE("arc::DataStore listener inside nested wrappers")
{
    arc::Graph<cluster::InLazyLazy, Root> graph;
    checkListener(graph.store, graph.probe->asTrait(trait::inspect), 7);
}

TEST_CASE("arc::DataStore listener inside arc::Union")
{
    arc::Graph<cluster::InUnion, Root> graph{.probe{std::in_place_type<Root::Probe>}};
    checkListener(graph.store, graph.probe->asTrait(trait::inspect), 7);
}

TEST_CASE("arc::DataStore non-listener in a cluster inside arc::Lazy")
{
    arc::Graph<cluster::WithLazySideOnly, Root> graph;
    REQUIRE(graph.store->add(2, 22).inserted);
    auto& side = graph.sub->side;
    CHECK(side->value(2) == 22);
    side->ping(2);
    CHECK(graph.probe->asTrait(trait::inspect).events() == 1);
}

TEST_CASE("arc::DataStore two listeners, one inside arc::Lazy")
{
    arc::Graph<cluster::OneOfTwoWrapped, Root> graph;
    REQUIRE(graph.store->add(1, 11).inserted);
    auto plain = graph.plain->asTrait(trait::inspect);
    auto wrapped = graph.wrapped->asTrait(trait::inspect);
    CHECK(plain.privateValue(1) == 7);
    CHECK(wrapped.privateValue(1) == 8);
    wrapped.touch(1);
    CHECK(plain.events() == 1);
    CHECK(wrapped.events() == 0);
    CHECK(wrapped.releaseOwn(1));
    CHECK(plain.releaseOwn(1));
    CHECK_FALSE(graph.store->get(1));
}

TEST_CASE("arc::DataStore in a subcluster, its listener outside")
{
    arc::Graph<cluster::OutsideStoreBox, Root> graph;
    checkListener(graph.box.store, graph.probe->asTrait(trait::inspect), 7);
}

} // namespace arc::tests::datastore::topology
