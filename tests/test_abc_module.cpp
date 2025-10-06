#include <doctest/doctest.h>

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

import arc;
import abc.graph;
import abc.ellie;

using namespace abc;

TEST_CASE("abc module")
{
    arc::Graph<AliceBob> g{.ellie{101}};
    g.onConstructed();
    auto aliceWrite = g.asTrait(trait::alice);
    aliceWrite.set(11);

    using AliceWrite = decltype(aliceWrite);
    static_assert(std::is_same_v<int, AliceWrite::Types::AliceType>);
    static_assert(aliceWrite.isTrait(trait::aliceWrite));
    static_assert(aliceWrite.isTrait(trait::aliceRead));

    CHECK(11 == g.alice.asTrait(trait::alice).get());
    CHECK(64 == g.alice.asTrait(trait::bob).get());
    CHECK(99 == g.alice.asTrait(trait::charlie).get());

    auto charlie1plus2 = g.charlie.asTrait(trait::charlie).get() + g.charlie.charlie.asTrait(trait::charlie2).get();
    CHECK(0 == charlie1plus2);

    CHECK(15 == g.charlie.charlie.asTrait(trait::charlie3).get());

    CHECK(64 == g.bob.asTrait(trait::bob).get());

    CHECK(99 == g.bob.asTrait(trait::charlie).get());

    CHECK(11 == g.charlie.charlie.asTrait(trait::aliceRead).get());

    CHECK(101 == g.ellie.asTrait(trait::ellie).get());

    CHECK(99 == g.ellie.asTrait(trait::charlie).get());

    CHECK(99 == g.ellie.asTrait(trait::charlie2).get());

    auto charlie2get = g.ellie.asTrait(trait::charlie2).get(arc::asFunctor);
    CHECK(99 == charlie2get());

    CHECK(101 == g.ellie->value);

    auto e2 = g.alice.getNode(trait::ellie2);
    static_assert(not g.ellie.hasTrait(trait::ellie2));
    static_assert(std::is_same_v<abc::EllieType, typename decltype(e2)::Types::EllieType>);
    CHECK(101 == e2.get());

    auto e3 = g.alice.getNode(trait::ellie3);
    static_assert(g.ellie.hasTrait(trait::ellie3));
    static_assert(std::is_same_v<abc::EllieType3, typename decltype(e3)::Types::EllieType>);
    CHECK(101 == e3.get());

    int visitableCounter = 0;
    g.visitTrait(trait::visitable, [&](auto v) { v.visit(visitableCounter); });
    CHECK(visitableCounter == 2);
}
