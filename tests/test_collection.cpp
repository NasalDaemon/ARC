#include <doctest/doctest.h>
#include "arc/macros.hpp"

// TODO: Why do we need to include ranges again with GCC?
#if !ARC_IMPORT_STD or ARC_COMPILER_GCC
#include <ranges>
#endif

import arc;
import arc.tests.collection;

namespace arc::tests::collection {

/* arc-embed-begin

export module arc.tests.collection;

namespace arc::tests::collection {

trait trait::Element
{
    get() const -> int
    count(int& counter) const
}

trait trait::Outside
{
    get() const -> int
}

trait trait::Global
{
    get() const -> int
}

cluster Element [R = Root]
{
    element = R::Element

    [trait::Outside <-> trait::Element]
    .. <-> element

    [trait::Global]
    element --> ^
}

cluster Collection [R = Root]
{
    outside = R::Outside
    collection = Element : arc::Collection<int>

    [trait::Element]
    .., outside --> collection

    [trait::Outside]
    collection --> outside
}

}

arc-embed-end */

struct ElementNode : arc::PeerNode
{
    using Depends = arc::Depends<trait::Outside>;
    using Traits = arc::Traits<ElementNode, trait::Element, arc::trait::Peer>;

    template<class Self>
    int impl(this Self const& self, trait::Element::get)
    {
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
        using Traits = arc::Traits<Node, trait::Outside>;

        int impl(trait::Outside::get) const
        {
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
    using Traits = arc::Traits<GlobalNode, trait::Global>;

    int impl(trait::Global::get) const
    {
        return i;
    }

    int i;
};

TEST_CASE("arc::Collection")
{
    struct Root
    {
        using Outside = OutsideNode;
        using Element = ElementNode;
    };
    arc::GraphWithGlobal<Collection, GlobalNode, Root> g{
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

}
