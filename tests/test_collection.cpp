#include "arc/doctest_prelude.h"
#include "arc/macros.hpp"

#include <version>

// GCC 15 ICEs (cp_emit_debug_info_for_using) instantiating std::flat_map across modules
#if __cpp_lib_flat_map and not ARC_COMPILER_LT(GCC, 16)
#define ARC_TEST_FLAT_MAP 1
#else
#define ARC_TEST_FLAT_MAP 0
#endif

// TODO: Why do we need to include ranges again with GCC?
#if !ARC_IMPORT_STD or ARC_COMPILER_GCC
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>
#if ARC_TEST_FLAT_MAP
#include <flat_map>
#endif
#endif

import arc;
import arc.tests.collection;

#include "arc/doctest.h"

namespace arc::tests::collection {

/* arc-begin
module;
#include <map>
export module arc.tests.collection;

namespace arc::tests::collection {

trait Element
{
    get() const -> int
    count(int& counter) const
}

trait Outside
{
    get() const -> int
}

trait Global
{
    get() const -> int
}

cluster Element [R = Root]
{
    element = R::Element

    // Explicitly allow getGlobal(trait::global)
    [trait::Global] @global

    [trait::Outside <-> trait::Element]
    .. <-> element

    // [trait::Global]
    // element --> ^
}

cluster LinearMapCollection [R = Root]
{
    outside = R::Outside
    collection = Element : arc::StaticLinearMap<int>

    [trait::Element]
    .., outside --> collection

    [trait::Outside]
    collection --> outside
}

cluster MapCollection [R = Root]
{
    outside = R::Outside
    collection = Element : arc::StaticMap<int>

    [trait::Element]
    .., outside --> collection

    [trait::Outside]
    collection --> outside
}

cluster OrderedDynamicMapCollection [R = Root]
{
    outside = R::Outside
    collection = Element : arc::DynamicMap<int, std::map>

    [trait::Element]
    .., outside --> collection

    [trait::Outside]
    collection --> outside
}

cluster RootCollection [R = Root]
{
    outside = R::Outside
    collection = R::Collection

    [trait::Element]
    .., outside --> collection

    [trait::Outside]
    collection --> outside
}

cluster StaticIndexCollection [R = Root]
{
    outside = R::Outside
    collection = Element : arc::StaticIndex<unsigned>

    [trait::Element]
    .., outside --> collection

    [trait::Outside]
    collection --> outside
}

cluster DynamicIndexCollection [R = Root]
{
    outside = R::Outside
    collection = Element : arc::DynamicIndex<unsigned>

    [trait::Element]
    .., outside --> collection

    [trait::Outside]
    collection --> outside
}

}

arc-end */

struct ElementNode : arc::PeerNode
{
    using Depends = arc::Depends<trait::Outside>;
    using Traits = arc::Traits<trait::Element, arc::trait::Peer>;

    template<class Self>
    int impl(this Self const& self, trait::Element::get)
    {
        static_assert(std::is_same_v<arc::InnerNodeHandle<arc::ContextOf<Self>>, ElementNode>);
        int peerCount = 0;
        for (auto& peer : self.getPeers())
        {
            peerCount++;
            CHECK(self.i != peer->i);
        }
        CHECK(peerCount > 0);
        return self.getNode(trait::outside).get() + self.i;
    }

    void impl(trait::Element::count, int& counter) const
    {
        counter++;
    }

    template<class Self>
    bool impl(this Self const& self, arc::trait::Peer::isPeerId, auto id)
    {
        CHECK(id != self.getElementId());
        return true;
    }

    bool impl(this auto const& self, arc::trait::Peer::isPeerInstance, auto const& other)
    {
        CHECK(&self.getState() != &other.getState());
        return true;
    }

    int i;
};

struct OutsideNode
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Element>;
        using Traits = arc::Traits<trait::Outside>;

        int impl(trait::Outside::get) const
        {
            static_assert(std::is_same_v<arc::InnerNodeHandle<Context>, OutsideNode>);
            return i;
        }

        int getElement(auto id) const
        {
            return getNode(trait::element, key::Element(id)).get();
        }

        auto control()
        {
            return getNode(trait::element, key::collection);
        }

        // Through the control view's const methods
        std::vector<int> elementValues() const
        {
            auto const control = getNode(trait::element, key::collection);
            std::vector<int> values;
            control.forEach([&](int, auto const&, auto const& element) { values.push_back(element.get()); });
            return values;
        }
        int elementTwice(int id) const
        {
            auto const control = getNode(trait::element, key::collection);
            auto const found = control.findIf([&](int found, auto const&, auto const&) { return found == id; });
            return control.fromId(id).get() + control.fromHandle(found).get();
        }

        int i;
    };
};

struct GlobalNode : arc::Node
{
    using Traits = arc::Traits<trait::Global>;

    template<class Self>
    int impl(this Self const& self, trait::Global::get)
    {
        static_assert(std::is_same_v<arc::InnerNodeHandle<arc::ContextOf<Self>>, GlobalNode>);
        return self.i;
    }

    int i;
};

struct Root
{
    using Outside = OutsideNode;
    using Element = ElementNode;
};

// Relocates its entries on growth and erasure, and has no reserve
template<class K, class V>
struct VecMap
{
    using value_type = std::pair<K, V>;
    using Vec = std::vector<value_type>;
    using iterator = Vec::iterator;
    using const_iterator = Vec::const_iterator;

    iterator begin() { return entries.begin(); }
    iterator end() { return entries.end(); }
    const_iterator begin() const { return entries.begin(); }
    const_iterator end() const { return entries.end(); }

    iterator find(K const& key) { return std::ranges::find(entries, key, &value_type::first); }
    const_iterator find(K const& key) const { return std::ranges::find(entries, key, &value_type::first); }

    template<class Value>
    std::pair<iterator, bool> emplace(K const& key, Value&& value)
    {
        if (auto it = find(key); it != end())
            return {it, false};
        entries.emplace_back(key, std::forward<Value>(value));
        return {std::prev(entries.end()), true};
    }

    iterator erase(const_iterator it) { return entries.erase(it); }
    std::size_t size() const { return entries.size(); }

    Vec entries;
};

template<class K, class V>
using TwoParamUnorderedMap = std::unordered_map<K, V>;

// Chooses cluster::RootCollection's collection type, and so its map template
template<class Collection_>
struct RootWith : Root
{
    using Collection = Collection_;
};

template<class Cluster, class R = Root>
void checkStaticMap()
{
    arc::GraphWithGlobal<Cluster, GlobalNode, R> g{
        .global{ARC_EMPLACE({.i = 14})},
        .main{
            .outside{ARC_EMPLACE({.i = 10})},
            .collection{2, [](auto insert) {
                insert(0, ARC_EMPLACE({.element{ARC_EMPLACE({.i = 0})}}));
                insert(1, ARC_EMPLACE({.element{ARC_EMPLACE({.i = 1})}}));
            }},
        },
    };

    CHECK(g.asTrait(trait::element, key::Element(0))->getElementId() == 0);
    CHECK(g.asTrait(trait::element, key::Element(1))->getElementId() == 1);

    CHECK(g.asTrait(trait::element, key::Element(0)).get() == 10);
    CHECK(g.asTrait(trait::element, key::Element(1)).get() == 11);
    CHECK(g->outside->getElement(0) == 10);
    CHECK(g->outside->getElement(1) == 11);

    auto handle0 = g.asTrait(trait::element, key::Element(0))->getElementHandle();
    auto handle1 = g.asTrait(trait::element, key::Element(1))->getElementHandle();

    CHECK(g.asTrait(trait::element, key::Element(handle0)).get() == 10);
    CHECK(g.asTrait(trait::element, key::Element(handle1)).get() == 11);
    CHECK(g->outside->getElement(handle0) == 10);
    CHECK(g->outside->getElement(handle1) == 11);

    CHECK(g->collection->getId(0)->getNode(trait::outside).get() == 10);

    int count = 0;
    g->collection.asTrait(trait::element, key::allElements).count(count);
    CHECK(count == 2);

    count = 0;
    g->collection.asTrait(trait::element, key::Elements([](int id) { return id % 2 == 0;})).count(count);
    CHECK(count == 1);

    CHECK(g->collection->atId(0).element.getGlobal(trait::global).get() == 14);
    CHECK(g->collection->atId(0).element.getNode(trait::global).get() == 14);
}

template<class Cluster, class R = Root>
void checkStaticMapBuild()
{
    using G = arc::GraphWithGlobal<Cluster, GlobalNode, R>;
    auto const build = [](auto adder)
    {
        return G{
            .global{ARC_EMPLACE({.i = 14})},
            .main{
                .outside{ARC_EMPLACE({.i = 10})},
                .collection{8, adder},
            },
        };
    };

    // Built by the adder, looked up by id
    G g = build([](auto insert) {
        for (int id : {7, 3, 5, 1})
            insert(id, ARC_EMPLACE({.element{ARC_EMPLACE({.i = id * 10})}}));
    });
    for (int id : {7, 3, 5, 1})
    {
        CHECK(g->collection->contains(id));
        CHECK(g.asTrait(trait::element, key::Element(id)).get() == 10 + id * 10);
        CHECK(g->collection->getId(id) == &g->collection->atId(id));
    }
    CHECK(not g->collection->contains(2));
    CHECK(g->collection->getId(2) == nullptr);
    CHECK_THROWS_AS((void)g->collection->atId(2), std::out_of_range);

    // Handles reach the element they were taken from, whatever the map's order
    for (int id : {7, 3, 5, 1})
    {
        auto const handle = g.asTrait(trait::element, key::Element(id))->getElementHandle();
        CHECK(g.asTrait(trait::element, key::Element(handle))->getElementId() == id);
        CHECK(&g->collection->atHandle(handle) == &g->collection->atId(id));
        CHECK(g->outside->getElement(handle) == 10 + id * 10);
    }

    int count = 0;
    g->collection.asTrait(trait::element, key::allElements).count(count);
    CHECK(count == 4);

    // A duplicate id throws
    CHECK_THROWS_AS((void)build([](auto insert) {
        insert(1, ARC_EMPLACE({.element{ARC_EMPLACE({.i = 1})}}));
        insert(1, ARC_EMPLACE({.element{ARC_EMPLACE({.i = 2})}}));
    }), std::invalid_argument);

    // Exceeding the fixed capacity throws, as elements must never relocate
    CHECK_THROWS_AS((void)build([](auto insert) {
        for (int id = 0; id < 9; ++id)
            insert(id, ARC_EMPLACE({.element{ARC_EMPLACE({.i = id})}}));
    }), std::length_error);
}

template<class Cluster, class R = Root>
void checkStaticMapSuite()
{
    checkStaticMap<Cluster, R>();
    checkStaticMapBuild<Cluster, R>();
}

TEST_CASE("arc::StaticLinearMap")
{
    checkStaticMapSuite<cluster::LinearMapCollection>();
}

TEST_CASE("arc::StaticMap")
{
    SUBCASE("std::unordered_map")
    {
        checkStaticMapSuite<cluster::MapCollection>();
    }
    SUBCASE("std::map")
    {
        using StdMap = RootWith<arc::StaticMap<cluster::Element, int, std::map>>;
        checkStaticMapSuite<cluster::RootCollection, StdMap>();
    }
    SUBCASE("two-parameter alias of std::unordered_map")
    {
        using AliasMap = RootWith<arc::StaticMap<cluster::Element, int, TwoParamUnorderedMap>>;
        checkStaticMapSuite<cluster::RootCollection, AliasMap>();
    }
    SUBCASE("vector-backed map")
    {
        using VectorMap = RootWith<arc::StaticMap<cluster::Element, int, VecMap>>;
        checkStaticMapSuite<cluster::RootCollection, VectorMap>();
    }
#if ARC_TEST_FLAT_MAP
    SUBCASE("std::flat_map")
    {
        using FlatMap = RootWith<arc::StaticMap<cluster::Element, int, std::flat_map>>;
        checkStaticMapSuite<cluster::RootCollection, FlatMap>();
    }
#endif
}

template<class R>
void checkDynamicMap()
{
    arc::GraphWithGlobal<cluster::RootCollection, GlobalNode, R> g{
        .global{ARC_EMPLACE({.i = 14})},
        .main{
            .outside{ARC_EMPLACE({.i = 10})},
            .collection{},
        },
    };

    auto control = g->outside->control();

    REQUIRE(g->collection->insert(0, ARC_EMPLACE({.element{ARC_EMPLACE({.i = 0})}})));
    REQUIRE(g->collection->insert(1, ARC_EMPLACE({.element{ARC_EMPLACE({.i = 1})}})));

    CHECK(g->collection->contains(0));
    CHECK(g->collection->contains(1));
    CHECK(not g->collection->contains(2));
    CHECK(control.contains(1));
    CHECK(g.asTrait(trait::element, key::Element(0)).get() == 10);
    CHECK(g.asTrait(trait::element, key::Element(1)).get() == 11);

    auto* const element0 = &g->collection->atId(0);
    CHECK(g->collection->getId(0) == element0);
    CHECK(g->collection->getId(2) == nullptr);
    CHECK_THROWS_AS((void)g->collection->atId(2), std::out_of_range);

    // Duplicates
    CHECK_THROWS_AS((void)g->collection->insert(0, ARC_EMPLACE({.element{ARC_EMPLACE({.i = 5})}})), std::invalid_argument);
    {
        auto [element, inserted] = g->collection->tryInsert(0, ARC_EMPLACE({.element{ARC_EMPLACE({.i = 5})}}));
        CHECK(not inserted);
        CHECK(element == element0);
        auto [handle, controlInserted] = control.tryInsert(1, ARC_EMPLACE({.element{ARC_EMPLACE({.i = 5})}}));
        CHECK(not controlInserted);
        CHECK(control.getId(handle) == 1);
    }
    CHECK(g.asTrait(trait::element, key::Element(0)).get() == 10);

    // A strong handle taken before many inserts (forcing several rehashes or
    // relocations of the map's entries) still reaches the same element
    auto const strong0 = control.getStrongHandle(0);
    for (int id = 2; id < 2000; ++id)
        REQUIRE(g->collection->insert(id, ARC_EMPLACE({.element{ARC_EMPLACE({.i = id})}})));
    CHECK(&g->collection->atId(0) == element0);
    CHECK(g->collection->getId(0) == element0);
    CHECK(&g->collection->atHandle(strong0) == element0);
    CHECK(control.contains(strong0));
    CHECK(control.getId(strong0) == 0);
    CHECK(control.fromHandle(strong0).get() == 10);
    CHECK(g.asTrait(trait::element, key::Element(1999)).get() == 2009);

    int count = 0;
    g->collection.asTrait(trait::element, key::allElements).count(count);
    CHECK(count == 2000);

    // A weak handle is invalid after remove
    auto const weak1 = control.getWeakHandle(1);
    CHECK(not weak1.empty());
    CHECK(not weak1.lock().empty());
    CHECK(g->collection->remove(1));
    CHECK(not g->collection->contains(1));
    CHECK(not g->collection->remove(1));
    CHECK(weak1.lock().empty());

    // A strong handle keeps its element alive, but no longer belongs to the collection
    CHECK(control.remove(strong0));
    CHECK(not control.contains(strong0));
    CHECK(not control.contains(0));
    CHECK(not control.remove(strong0));
    CHECK(&g->collection->atHandle(strong0) == element0);
    CHECK(control.getId(strong0) == 0);

    g->collection->removeIf([](int id, auto const&, auto const&) { return id % 2 == 0; });
    count = 0;
    g->collection->forEach([&](int id, auto const&, auto const&) { CHECK(id % 2 == 1); ++count; });
    CHECK(count == 999);
}

TEST_CASE("arc::DynamicMap")
{
    SUBCASE("std::unordered_map")
    {
        checkDynamicMap<RootWith<arc::DynamicMap<cluster::Element, int>>>();
    }
    SUBCASE("std::map")
    {
        checkDynamicMap<RootWith<arc::DynamicMap<cluster::Element, int, std::map>>>();
    }
    SUBCASE("two-parameter alias of std::unordered_map")
    {
        checkDynamicMap<RootWith<arc::DynamicMap<cluster::Element, int, TwoParamUnorderedMap>>>();
    }
    SUBCASE("vector-backed map")
    {
        checkDynamicMap<RootWith<arc::DynamicMap<cluster::Element, int, VecMap>>>();
    }
#if ARC_TEST_FLAT_MAP
    SUBCASE("std::flat_map")
    {
        checkDynamicMap<RootWith<arc::DynamicMap<cluster::Element, int, std::flat_map>>>();
    }
#endif
}

TEST_CASE("arc::DynamicLinearMap")
{
    checkDynamicMap<RootWith<arc::DynamicLinearMap<cluster::Element, int>>>();
}

TEST_CASE("arc::DynamicMap over std::map iterates in key order")
{
    arc::GraphWithGlobal<cluster::OrderedDynamicMapCollection, GlobalNode, Root> g{
        .global{ARC_EMPLACE({.i = 14})},
        .main{
            .outside{ARC_EMPLACE({.i = 10})},
            .collection{},
        },
    };

    for (int id : {5, 1, 4, 2, 3})
        REQUIRE(g->collection->insert(id, ARC_EMPLACE({.element{ARC_EMPLACE({.i = id})}})));

    std::vector<int> ids;
    g->collection->forEach([&](int id, auto const&, auto const&) { ids.push_back(id); });
    CHECK(ids == std::vector{1, 2, 3, 4, 5});

    ids.clear();
    g->outside->control().forEach([&](int id, auto const&, auto const&) { ids.push_back(id); });
    CHECK(ids == std::vector{1, 2, 3, 4, 5});

    CHECK(g->outside->elementValues() == std::vector{11, 12, 13, 14, 15});
    CHECK(g->outside->elementTwice(4) == 28);
}

TEST_CASE("arc::StaticIndex")
{
    struct Root
    {
        using Outside = OutsideNode;
        using Element = ElementNode;
    };
    arc::GraphWithGlobal<cluster::StaticIndexCollection, GlobalNode, Root> g{
        .global{ARC_EMPLACE({.i = 14})},
        .main{
            .outside{ARC_EMPLACE({.i = 10})},
            .collection{2, [](auto insert) {
                insert(arc::autoId, ARC_EMPLACE({.element{ARC_EMPLACE({.i = 0})}}));
                insert(arc::autoId, ARC_EMPLACE({.element{ARC_EMPLACE({.i = 1})}}));
            }},
        },
    };

    // Ids are assigned sequentially from zero
    CHECK(g.asTrait(trait::element, key::Element(0u))->getElementId() == 0u);
    CHECK(g.asTrait(trait::element, key::Element(1u))->getElementId() == 1u);
    CHECK(g.asTrait(trait::element, key::Element(0u)).get() == 10);
    CHECK(g.asTrait(trait::element, key::Element(1u)).get() == 11);
    CHECK(g->outside->getElement(0u) == 10);

    CHECK(g->collection->contains(0u));
    CHECK(g->collection->contains(1u));
    CHECK(not g->collection->contains(2u));

    auto handle0 = g.asTrait(trait::element, key::Element(0u))->getElementHandle();
    CHECK(g.asTrait(trait::element, key::Element(handle0)).get() == 10);

    int count = 0;
    g->collection.asTrait(trait::element, key::allElements).count(count);
    CHECK(count == 2);
}

TEST_CASE("arc::DynamicIndex")
{
    struct Root
    {
        using Outside = OutsideNode;
        using Element = ElementNode;
    };
    arc::GraphWithGlobal<cluster::DynamicIndexCollection, GlobalNode, Root> g{
        .global{ARC_EMPLACE({.i = 14})},
        .main{
            .outside{ARC_EMPLACE({.i = 10})},
            .collection{},
        },
    };

    REQUIRE(g->collection->insert(arc::autoId, ARC_EMPLACE({.element{ARC_EMPLACE({.i = 0})}})));
    REQUIRE(g->collection->insert(arc::autoId, ARC_EMPLACE({.element{ARC_EMPLACE({.i = 1})}})));

    CHECK(g.asTrait(trait::element, key::Element(0u)).get() == 10);
    CHECK(g.asTrait(trait::element, key::Element(1u)).get() == 11);

    CHECK(g->collection->remove(0u));
    CHECK(not g->collection->contains(0u));
    CHECK(not g->collection->remove(0u));

    // Freed index is reused
    REQUIRE(g->collection->insert(arc::autoId, ARC_EMPLACE({.element{ARC_EMPLACE({.i = 5})}})));
    CHECK(g->collection->contains(0u));
    CHECK(g.asTrait(trait::element, key::Element(0u)).get() == 15);

    int count = 0;
    g->collection.asTrait(trait::element, key::allElements).count(count);
    CHECK(count == 2);
}

namespace {
    struct SmallElem : std::enable_shared_from_this<SmallElem>
    {
        explicit SmallElem(std::uint8_t id) : id(id) {}
        std::uint8_t id;
    };
}

TEST_CASE("arc::IndexStorage id exhaustion")
{
    arc::IndexStorage<SmallElem, std::uint8_t, true> store;

    for (int i = 0; i < 256; ++i)
        REQUIRE(store.emplace(arc::autoId) != nullptr);

    // Id space exhausted: emplace refuses, ensureSpareCapacity throws
    CHECK(store.emplace(arc::autoId) == nullptr);
    CHECK_THROWS_AS(store.ensureSpareCapacity(), std::length_error);

    // A freed id restores spare capacity and is reused
    CHECK(store.erase(std::uint8_t{7}));
    CHECK_NOTHROW(store.ensureSpareCapacity());
    auto* el = store.emplace(arc::autoId);
    REQUIRE(el != nullptr);
    CHECK(el->id == 7);

    // eraseIf must terminate over an exactly-full id space
    store.eraseIf([](SmallElem const&) { return true; });
    CHECK(store.size() == 0);
}

namespace {
    struct IndexedElem : std::enable_shared_from_this<IndexedElem>
    {
        explicit IndexedElem(unsigned id, int v) : id(id), v(v) {}
        unsigned id;
        int v;
    };
}

TEST_CASE("arc::IndexStorage")
{
    static_assert(arc::IsCollectionStorage<arc::IndexStorage<IndexedElem, unsigned, false>, IndexedElem, unsigned>);
    static_assert(arc::IsDynamicCollectionStorage<arc::IndexStorage<IndexedElem, unsigned, true>, IndexedElem, unsigned>);

    arc::IndexStorage<IndexedElem, unsigned, true> store;

    auto* a = store.emplace(arc::autoId, 1);
    auto* b = store.emplace(arc::autoId, 2);
    CHECK(a->id == 0u);
    CHECK(b->id == 1u);
    CHECK(store.size() == 2);

    CHECK(store.erase(0u));
    CHECK(not store.erase(0u));
    CHECK(store.findById(0u) == nullptr);
    CHECK(not store.contains(0u));
    CHECK(store.size() == 1);

    // Freed index is reused
    auto* c = store.emplace(arc::autoId, 3);
    CHECK(c->id == 0u);
    CHECK(c->v == 3);
    CHECK(store.size() == 2);

    auto handle = store.strongHandleFor(1u);
    CHECK(store.idOf(handle) == 1u);
    CHECK(store.elementOf(handle)->v == 2);
    CHECK(store.contains(handle));

    int sum = 0;
    for (auto const& el : store.view())
        sum += el->v;
    CHECK(sum == 5);

    store.eraseIf([](IndexedElem const& el) { return el.v == 3; });
    CHECK(not store.contains(0u));
    CHECK(store.size() == 1);
}

namespace {
    template<class K, class V>
    struct ThrowOn13Map : VecMap<K, V>
    {
        template<class Value>
        auto emplace(K const& key, Value&& value)
        {
            if (key == 13)
                throw std::runtime_error("ThrowOn13Map");
            return VecMap<K, V>::emplace(key, std::forward<Value>(value));
        }
    };

    struct MoveOnlyElem : std::enable_shared_from_this<MoveOnlyElem>
    {
        MoveOnlyElem(unsigned id, int v) : id(id), v(std::make_unique<int>(v)) {}
        unsigned id;
        std::unique_ptr<int> v;
    };

    struct ProbedElem : std::enable_shared_from_this<ProbedElem>
    {
        ProbedElem(unsigned id, std::function<void(unsigned)> const* onDestroy) : id(id), onDestroy(onDestroy) {}
        ~ProbedElem() { (*onDestroy)(id); }
        unsigned id;
        std::function<void(unsigned)> const* onDestroy;
    };
}

TEST_CASE("arc::MapStorage")
{
    static_assert(arc::IsCollectionStorage<arc::MapStorage<IndexedElem, unsigned, false>, IndexedElem, unsigned>);
    static_assert(arc::IsDynamicCollectionStorage<arc::MapStorage<IndexedElem, unsigned, true>, IndexedElem, unsigned>);
    static_assert(arc::IsDynamicCollectionStorage<arc::MapStorage<IndexedElem, unsigned, true, std::map>, IndexedElem, unsigned>);
    static_assert(arc::IsDynamicCollectionStorage<arc::MapStorage<IndexedElem, unsigned, true, VecMap>, IndexedElem, unsigned>);
    static_assert(arc::IsCollectionStorage<arc::MapStorage<IndexedElem, unsigned, false, VecMap>, IndexedElem, unsigned>);

    SUBCASE("dynamic")
    {
        arc::MapStorage<IndexedElem, unsigned, true, std::map> store;
        auto* a = store.emplace(4u, 1);
        auto* b = store.emplace(2u, 2);
        CHECK(store.size() == 2);
        CHECK(store.findById(4u) == a);
        CHECK(std::as_const(store).findById(2u) == b);
        CHECK(store.findById(3u) == nullptr);

        auto const handle = store.strongHandleFor(4u);
        CHECK(store.contains(handle));
        CHECK(store.idOf(handle) == 4u);
        CHECK(store.elementOf(handle) == a);
        CHECK(store.handleFor(a) == handle);

        std::vector<unsigned> ids;
        for (auto const& el : store.view())
            ids.push_back(el->id);
        CHECK(ids == std::vector{2u, 4u});

        CHECK(store.erase(4u));
        CHECK(not store.erase(4u));
        CHECK(not store.contains(handle));
        // The handle still owns its element
        CHECK(store.elementOf(handle)->v == 1);

        store.eraseIf([](IndexedElem const& el) { return el.v == 2; });
        CHECK(store.size() == 0);
    }

    SUBCASE("static")
    {
        arc::MapStorage<IndexedElem, unsigned, false, std::map> store;
        store.reserve(2);
        auto* a = store.emplace(9u, 1);
        auto* b = store.emplace(3u, 2);
        CHECK_THROWS_AS(store.ensureSpareCapacity(), std::length_error);
        CHECK(store.findById(9u) == a);
        CHECK(store.findById(3u) == b);
        CHECK(store.contains(9u));
        CHECK(not store.contains(4u));
        CHECK(store.contains(store.handleFor(a)));
        CHECK(store.elementOf(store.handleFor(a)) == a);

        // Iteration follows the map's (key) order, not insertion order
        std::vector<unsigned> ids;
        for (auto const* el : store.view())
            ids.push_back(el->id);
        CHECK(ids == std::vector{3u, 9u});
    }

    SUBCASE("move-only elements")
    {
        arc::MapStorage<MoveOnlyElem, unsigned, false> staticStore;
        staticStore.reserve(2);
        staticStore.emplace(1u, 10);
        staticStore.emplace(2u, 20);
        CHECK(*staticStore.findById(2u)->v == 20);

        arc::MapStorage<MoveOnlyElem, unsigned, true> dynamicStore;
        dynamicStore.emplace(1u, 10);
        dynamicStore.emplace(2u, 20);
        CHECK(*dynamicStore.findById(2u)->v == 20);
    }

    SUBCASE("static emplace rolls back when the map throws")
    {
        arc::MapStorage<IndexedElem, unsigned, false, ThrowOn13Map> store;
        store.reserve(4);
        auto* a = store.emplace(1u, 1);
        CHECK_THROWS_AS(store.emplace(13u, 2), std::runtime_error);
        CHECK(store.size() == 1);
        CHECK(store.findById(13u) == nullptr);
        CHECK(store.findById(1u) == a);
        std::vector<unsigned> ids;
        for (auto const* el : store.view())
            ids.push_back(el->id);
        CHECK(ids == std::vector{1u});
    }

    SUBCASE("dynamic erasure destroys elements once the map is consistent")
    {
        struct Seen
        {
            unsigned id;
            bool contained;
            std::size_t size;
        };
        std::vector<Seen> seen;
        std::function<void(unsigned)> onDestroy;
        arc::MapStorage<ProbedElem, unsigned, true, std::map> store;
        onDestroy = [&](unsigned id) { seen.push_back({id, store.contains(id), store.size()}); };

        for (unsigned id : {1u, 2u, 3u})
            store.emplace(id, &onDestroy);

        CHECK(store.erase(2u));
        REQUIRE(seen.size() == 1);
        CHECK(seen[0].id == 2u);
        CHECK(not seen[0].contained);
        CHECK(seen[0].size == 2);

        store.eraseIf([](ProbedElem const& el) { return el.id == 1u; });
        REQUIRE(seen.size() == 2);
        CHECK(seen[1].id == 1u);
        CHECK(not seen[1].contained);
        CHECK(seen[1].size == 1);

        store.eraseIf([](ProbedElem const&) { return true; });
        CHECK(store.size() == 0);
    }
}

} // namespace arc::tests::collection
