#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <any>
#include <span>
#include <variant>
#include <vector>
#endif

import arc.tests.invoke_method;

import arc;

#include "arc/doctest.h"

namespace arc::tests::invoke_method {

/* arc-begin

export module arc.tests.invoke_method;

trait arc::tests::invoke_method::Trait [Types]
{
    type Tracker

    returnsTracker() const -> Types::Tracker
}

arc-end */

struct CtrTracker
{
    CtrTracker() = default;
    CtrTracker(CtrTracker const& o) : copies(o.copies + 1), moves(o.moves) {}
    CtrTracker(CtrTracker&& o) : copies(o.copies), moves(o.moves + 1) {}

    int copies = 0;
    int moves = 0;
};

struct Node : arc::NodeImpl<Trait>
{
    struct Types
    {
        using Tracker = invoke_method::CtrTracker;
    };

    CtrTracker returnsTracker() const
    {
        return {};
    }
};

TEST_CASE("Method returns with RTO")
{
    arc::test::Graph<Node> g;
    auto node = g.node.asTrait(Trait{});

    std::same_as<CtrTracker> auto result = node.returnsTracker();
    CHECK(0 == result.copies);
#if not ARC_COMPILER_CLANG
    CHECK(0 == result.moves);
#endif
}

} // namespace arc::tests::invoke_method
