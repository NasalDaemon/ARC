#include <doctest/doctest.h>
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <any>
#include <typeinfo>
#include <variant>
#include <vector>
#endif

import arc;

namespace arc::tests::lazy {

struct Trait : arc::UnconstrainedTrait
{
    #define ARC_METHODS_Trait(TAG) \
        TAG(get)

    ARC_METHODS(Trait)
} inline constexpr trait{};

struct Node : arc::Node
{
    using Traits = arc::Traits<Node, Trait>;

    int impl(this auto& self, Trait::get)
    {
        return self.i + int(self.getNode(trait).get());
    }

    Node(int i = 88) : i(i) {}

    int i;
};

TEST_CASE("std::variant element offset")
{
    std::variant<int, double> v{9};
    CHECK(static_cast<void*>(&v) == static_cast<void*>(&std::get<int>(v)));
}

TEST_CASE("arc::Lazy")
{
    arc::test::Graph<arc::Lazy<Node>> g{.node{33}};
    g.mocks->define([](Trait::get) { return 10; });
    auto g2 = std::move(g);
    auto g3 = g2;

    CHECK(g3.node->i == 33);
    CHECK(g3.node.asTrait(trait).get() == 43);
}

}
