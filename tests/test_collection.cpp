#include "arc/macros.hpp"

// TODO: Why do we need to include ranges again with GCC?
#if !ARC_IMPORT_STD or ARC_COMPILER_GCC
#include <cstdint>
#include <memory>
#include <ranges>
#endif

import arc;
import arc.tests.collection;

#include "arc/doctest.h"

namespace arc::tests::collection {

/* arc-begin

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

cluster Collection [R = Root]
{
    outside = R::Outside
    collection = Element : arc::StaticMap<int>

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

TEST_CASE("arc::StaticMap")
{
    struct Root
    {
        using Outside = OutsideNode;
        using Element = ElementNode;
    };
    arc::GraphWithGlobal<cluster::Collection, GlobalNode, Root> g{
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

} // namespace arc::tests::collection
