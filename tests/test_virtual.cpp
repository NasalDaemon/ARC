#include <doctest/doctest.h>
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <memory>
#include <type_traits>
#include <utility>
#endif

import arc.tests.virtual_;
import arc;


/* arc-embed-begin

export module arc.tests.virtual_;

namespace arc::tests::virtual_ {

trait trait::Apple
{
    seeds() const -> int
    testExchange() -> bool
}

trait trait::Bread
{
    slices() const -> int
}

trait trait::Egg
{
    yolks() const -> int
}

trait trait::Global
{
    get() const -> int
}

arc-embed-end */

namespace arc::tests::virtual_ {

struct AppleType;
struct BreadType;

struct IApple : arc::INodeImpl<trait::Apple>
{
    struct Types
    {
        using AppleType = virtual_::AppleType;
    };
    virtual int seeds() const = 0;
    virtual bool testExchange() = 0;
};
struct IBread : arc::INodeImpl<trait::Bread*>
{
    struct Types
    {
        using BreadType = virtual_::BreadType;
    };
    virtual int impl(trait::Bread::slices) const = 0;
};
struct IEgg : arc::INodeImpl<trait::Egg>
{
    virtual int yolks() const = 0;
};

struct AppleEgg
{
    template<class Context>
    struct Node : arc::INodeOf<IApple, IEgg>::Uses<trait::Bread>
    {
        static_assert(std::is_same_v<BreadType, typename arc::ResolveTypes<Node, trait::Bread>::BreadType>);

        int seeds() const final
        {
            asEgg();
            return seeds_ + getBread().slices();
        }

        int yolks() const final
        {
            return yolks_;
        }

        bool testExchange() final
        {
            if constexpr (arc::IsVirtualContext<Context>)
            {
                arc::KeepAlive keepAlive;
                {
                    CHECK(asApple().seeds() == 34);
                    allowDestroy = false;
                    auto handle = exchangeImpl<AppleEgg>(11, 3);
                    REQUIRE(handle.getNext() != this);
                    CHECK(handle.getNext()->asApple().seeds() == 47);
                    CHECK(handle.getNext()->seeds() == 47);
                    // Using current node still works, but "re-entry" to this context from apple
                    // will go to the new node that was just emplaced into the graph instead
                    CHECK(asApple().seeds() == 46);
                    keepAlive = handle;
                }
                CHECK(asApple().seeds() == 46);
                CHECK(seeds() == 46);
                allowDestroy = true;
                return true;
            }
            return false;
        }

        explicit Node(int seeds = 10, int yolks = 2)
            : seeds_(seeds), yolks_(yolks)
        {}

        ~Node()
        {
            REQUIRE(allowDestroy);
        }

        int seeds_;
        int yolks_;
        bool allowDestroy = true;
    };
};

struct Bread
{
    template<class Context>
    struct Node : arc::Uses<IBread, trait::Apple, trait::Egg, arc::Global<trait::Global>>
    {
        // Prove that withFactory works with copy/move elision
        Node() = default;
        Node(Node const&) = delete;
        Node(Node&&) = delete;

        static_assert(std::is_same_v<AppleType, typename arc::ResolveTypes<Node, trait::Apple>::AppleType>);

        int impl(trait::Bread::slices) const final
        {
            static_assert(requires { Bread::slices(); });
            return getEgg().yolks() * slices;
        }

        void onGraphConstructed()
        {
            // Proves getGlobal isn't a converter in Uses, as it clashes with Node::getGlobal
            CHECK(getGlobal(trait::global).get() == 9);
            CHECK(getGlobalGlobal().get() == 9);
            CHECK(slices == 314);
            slices = 12;
        }

        int slices = 314;
    };
};

struct GlobalNode : arc::Node
{
    using Traits = arc::Traits<trait::Global>;

    int impl(trait::Global::get) const
    {
        return i;
    }

    int i = 9;
};

void test(auto& g)
{
    g.onConstructed();

    CHECK(g.asTrait(trait::apple).seeds() == 34);
    CHECK(g.asTrait(trait::egg).yolks() == 2);
    CHECK(g->mocks.asTrait(trait::bread).slices() == 24);

    int count = 0;
    g.visitTrait(
        trait::apple,
        [&](auto apple)
        {
            count++;
            CHECK(apple.seeds() == 34);
        });
    CHECK(count == 1);
}

void testExchange(auto& g)
{
    auto g2 = std::move(g);
    CHECK(g2.asTrait(trait::apple).seeds() == 34);
    CHECK(g2.asTrait(trait::egg).yolks() == 2);
    CHECK(g2->mocks.asTrait(trait::bread).slices() == 24);

    CHECK(g2.asTrait(trait::apple).testExchange());
}

TEST_CASE("arc::Virtual")
{
    using Virtual = arc::test::GraphWithGlobal<arc::Virtual<IApple, IEgg>, GlobalNode, arc::Virtual<IBread>>;
    Virtual virt{
        .global{ARC_EMPLACE({.i = 9})},
        .main{
            .node{std::in_place_type<AppleEgg>},
            .mocks{arc::withFactory, []<class C>(C) -> C::type { return std::in_place_type<Bread>; }},
        },
    };
    test(virt);
    testExchange(virt);

    using Static = arc::test::GraphWithGlobal<AppleEgg, GlobalNode, Bread>;
    Static stat;
    test(stat);
}

struct VirtualOnly
{
    struct Leaf final : arc::INodeOf<IEgg, IBread>
    {
        int yolks() const
        {
            static_assert(requires { yolks(); });
            return yolks_;
        }
        int impl(trait::Bread::slices) const
        {
            static_assert(requires { slices(); });
            static_assert(requires { Bread::slices(); });
            return slices_;
        }

        explicit Leaf(int yolks = 999, int slices = 88)
            : yolks_(yolks), slices_(slices)
        {}

        int yolks_;
        int slices_;
    };

    template<class Context>
    using Node = Leaf;
};

TEST_CASE("arc::Virtual: Virtual-only node may be final without traits and simple leaf")
{
    arc::test::Graph<arc::Virtual<IBread, IEgg>> g{.node{std::in_place_type<VirtualOnly::Leaf>}};

    CHECK(g.asTrait(trait::egg).yolks() == 999);
    CHECK(g.asTrait(trait::bread).slices() == 88);

    g.node->emplace<VirtualOnly>(11, 333);

    CHECK(g.asTrait(trait::egg).yolks() == 11);
    CHECK(g.asTrait(trait::bread).slices() == 333);
}

struct StaticBread
{
    template<class Context>
    struct Node : arc::Node
    {
        using Traits = arc::Traits<trait::Bread>;

        int impl(trait::Bread::slices) const
        {
            return slices + getNode(trait::egg).yolks();
        }

        explicit Node(int slices = 42) : slices(slices) {}

        int slices;
    };
};

struct BreadFacade
{
    template<class Context>
    struct Node final : arc::INodeOf<IApple, IBread>
    {
        int seeds() const { return 0; }
        bool testExchange()
        {
            if constexpr (arc::IsVirtualContext<Context>)
            {
                auto handle = exchangeImpl<arc::Adapt<StaticBread, BreadFacade>>();
                int expected = 99 + 42 + 1;
                CHECK(asTrait(trait::bread).slices() == expected);
                CHECK(handle.getNext()->asTrait(trait::bread).slices() == expected);
                return true;
            }
            return false;
        }

        int impl(trait::Bread::slices) const
        {
            return test + getNode(trait::bread).slices();
        }

        explicit Node(int test = 99) : test(test) {}

        int test;
    };
};

struct EggDouble
{
    template<class Context>
    struct Node : arc::Node
    {
        using Traits = arc::Traits<trait::Egg>;

        int impl(trait::Egg::yolks) const { return yolks; }

        explicit Node(int yolks = 1) : yolks(yolks) {}
        int yolks;
    };
};

TEST_CASE("arc::Virtual with arc::Adapt")
{
    arc::test::Graph<arc::Virtual<IApple, IBread>, EggDouble> g{
        .node{arc::adapt<StaticBread, BreadFacade>}
    };

    int expected = 99 + 42 + 1;
    CHECK(g.asTrait(trait::bread).slices() == expected);
    CHECK(g.asTrait(trait::apple).testExchange());
}

struct EggFacade
{
    template<class Context>
    struct Node final : IEgg
    {
        int yolks() const
        {
            return getNode(trait::egg).yolks();
        }
    };
};

/* arc-embed-begin

cluster EggBread [R = Root]
{
    egg = R::StaticEgg
    bread = R::VirtualBread

    [trait::Egg <-> trait::Bread]
    egg <-> bread
}

arc-embed-end */

ARC_INSTANTIATE_BOX(StaticBread, BreadFacade, EggFacade, IEgg)

TEST_CASE("arc::Box")
{
    struct Root
    {
        using StaticEgg = virtual_::EggDouble;
        using VirtualBread = arc::Virtual<IBread>;
    };

    arc::Graph<cluster::EggBread, Root> g{
        .bread{arc::box<StaticBread, BreadFacade, EggFacade, IEgg>, arc::args<StaticBread>(41), arc::args<BreadFacade>(98)}
    };

    CHECK(g.bread.asTrait(trait::bread).slices() == 140);

    auto g2 = std::move(g);
    CHECK(g2.bread.asTrait(trait::bread).slices() == 140);
}

ARC_INSTANTIATE_BOX(EggDouble, EggFacade)

TEST_CASE("arc::Box with no outnode")
{
    arc::test::Graph<arc::Virtual<IEgg>> g{
        .node{arc::box<EggDouble, EggFacade>, 4}
    };

    CHECK(g.node.asTrait(trait::egg).yolks() == 4);

    auto g2 = std::move(g);
    CHECK(g2.node.asTrait(trait::egg).yolks() == 4);
}

// arc-embed-begin

} // namespace arc::tests::virtual_

// arc-embed-end
