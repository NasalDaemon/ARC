#include <doctest/doctest.h>
#include "arc/macros.hpp"

// TODO: Why do we need to include ranges again with GCC?
#if !ARC_IMPORT_STD or ARC_COMPILER_GCC
#include <ranges>
#endif

import arc.tests.thread.collection;
import arc.tests.thread.global_scheduler;
import arc.tests.thread.poster;
import arc;

/* arc-embed-begin

export module arc.tests.thread.collection;

namespace arc::tests::thread::trait {

trait Inner
{
    get() const -> int
    getPeer(int index) const -> int
    getInner() const -> int
}

trait Outer
{
    get() const -> int
}

}

namespace arc::tests::thread {

cluster Inner [R = Root]
{
    node1 = R::Inner
    node2 = R::Inner

    [trait::Outer <-> trait::Inner]
    .. <-> node1 --> node2
    .. <------------ node2
}

cluster CollectionCluster [R = Root]
{
    collection = Inner : OnDynThread : arc::Collection<int>
    outer = R::Outer : arc::OnDynThread

    [trait::Inner <-> trait::Outer]
    collection <-> outer
}

}

arc-embed-end */

namespace arc::tests::thread {

using namespace arc::tests::thread::trait;

struct InnerNode : arc::PeerNode
{
    struct OuterDetached;
    struct PeerDetached;
    using Traits = arc::Traits<InnerNode
        , trait::Inner
        , trait::Outer(OuterDetached)
        , arc::trait::Peer(arc::PeerDetachedOpen)
    >;
    using Depends = arc::Depends<trait::Outer, trait::Inner*>;

    explicit InnerNode(int i) : i(i) {}

    int impl(trait::Inner::get) const
    {
        return i;
    }

    int impl(this auto const& self, trait::Inner::getPeer, int index)
    {
        for (auto& peer : self.getPeers())
        {
            static_assert(std::is_const_v<std::remove_reference_t<decltype(peer)>>);
            CHECK_THROWS(peer.asTrait(trait::inner).get());
            // can always get the element id as it never changes during the lifetime of the node
            if (peer.getElementId() == index)
            {
                return peer.asTrait(trait::inner, future).get().get();
            }
        }
        throw std::out_of_range("No peer with the given index found");
    }

    int impl(this auto const& self, trait::Inner::getInner)
    {
        return self.getNode(trait::inner).get();
    }

    struct OuterDetached : arc::DetachedInterface
    {
        int impl(this auto const& self, trait::Outer::get)
        {
            return self.getNode(trait::outer, future).get().get();
        }
    };

    int i;
};

struct OuterNode : arc::Node
{
    using Traits = arc::Traits<OuterNode, trait::Outer>;
    using Depends = arc::Depends<trait::Inner>;

    explicit OuterNode(int threadId) : i(threadId) {}

    int impl(trait::Outer::get) const
    {
        return i;
    }

    int impl(this auto const& self, trait::Inner::get)
    {
        return self.getNode(trait::inner, arc::key::Element(0)).get();
    }

    int i;
};

TEST_CASE("arc::Collection using threads")
{
    struct Root
    {
        using Inner = InnerNode;
        using Outer = OuterNode;
    };

    arc::GraphWithGlobal<CollectionCluster, GlobalScheduler, Root> graph{
        .global{
            [](auto& scheduler)
            {
                scheduler.addThread();
                scheduler.addThread();
                scheduler.addThread();
            }
        },
        .main{
            .collection{3,
                [](auto add)
                {
                    add(0, 0, ARC_EMPLACE(
                        .node1{314},
                        .node2{315},
                    ));
                    add(1, 1, ARC_EMPLACE(
                        .node1{42},
                        .node2{43},
                    ));
                    add(2, 2, ARC_EMPLACE(
                        .node1{99},
                        .node2{100},
                    ));
                }
            },
            .outer{3, 101},
        },
    };

    auto scheduler = graph.global.asTrait(arc::trait::scheduler);

    auto mainTask = [&]
    {
        CHECK(scheduler.isCurrentThread(0));
        CHECK(not scheduler.inExclusiveMode());
        auto& at0 = graph->collection->atId(0);
        auto& at1 = graph->collection->atId(1);
        auto& at2 = graph->collection->atId(2);

        CHECK(at0->asTrait(trait::inner).get() == 314);
        CHECK_THROWS(at1->asTrait(trait::inner).get());
        CHECK_THROWS(at2->asTrait(trait::inner).get());

        CHECK(graph->collection->getId(0)->asTrait(trait::inner, future).get().get() == 314);
        CHECK(graph->collection->getId(1)->asTrait(trait::inner, future).get().get() == 42);
        CHECK(graph->collection->getId(2)->asTrait(trait::inner, future).get().get() == 99);

        CHECK(graph->outer.asTrait(trait::outer, future).get().get() == 101);
        CHECK(at0->node1.asTrait(trait::outer).get() == 101);
        CHECK(at1->node1.asTrait(trait::outer).get() == 101);
        CHECK(at2->node1.asTrait(trait::outer).get() == 101);

        // Cannot get self as peer node
        CHECK_THROWS_WITH(at0->asTrait(trait::inner, future).getPeer(0).get(),
            "No peer with the given index found");
        CHECK(at0->asTrait(trait::inner, future).getPeer(1).get() == 42);
        CHECK(at0->asTrait(trait::inner, future).getPeer(2).get() == 99);

        CHECK(at0->asTrait(trait::inner).getInner() == 315);
        CHECK(at1->asTrait(trait::inner, future).getInner().get() == 43);
        CHECK(at2->asTrait(trait::inner, future).getInner().get() == 100);

        scheduler.stop();
    };

    CHECK(scheduler.postTask(0, mainTask));
    scheduler.run();
}

}
