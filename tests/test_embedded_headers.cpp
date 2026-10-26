#include "arc/tests/test_embedded_headers.hxx"

#include "arc/arc.hpp"

#include <doctest/doctest.h>

#if 0
// arc-begin
trait arc::tests::trait::Embedded
{
    get() const
}
// arc-end

random code between two sections will be ignored

// arc-begin
cluster arc::tests::EmbeddedCluster [R = Root]
{
    embedded = R::Embedded

    [trait::Embedded]
    .. --> embedded
}
// arc-end

// arc-begin
cluster arc::tests::CustomCtorCluster [R = Root]
{
    first = R::CtorNode
    second = R::CtorNode

    CustomCtorCluster() : CustomCtorCluster(41)
    CustomCtorCluster(int value)
        .first{value}
        .second{value + 1}
    CustomCtorCluster(int value, bool unused)
        .first{value}
}
// arc-end
#endif

namespace arc::tests {

struct EmbeddedNode : arc::Node
{
    using Traits = arc::Traits<trait::Embedded>;

    int impl(trait::Embedded::get) const { return 42; }
};

struct CtorNode : arc::Node
{
    using Traits = arc::Traits<>;

    int value{};

    constexpr CtorNode() = default;
    constexpr explicit CtorNode(int value) : value{value} {}
};

} // namespace arc::tests

using namespace arc::tests;

TEST_CASE("embedded")
{
    struct Root
    {
        using Embedded = EmbeddedNode;
    };
    arc::Graph<cluster::EmbeddedCluster, Root> g{};

    CHECK(g.asTrait(trait::embedded).get() == 42);
}

TEST_CASE("cluster custom constructors")
{
    struct Root
    {
        using CtorNode = arc::tests::CtorNode;
    };
    using Graph = arc::Graph<cluster::CustomCtorCluster, Root>;

    Graph designated{7};
    CHECK(designated.first->value == 7);
    CHECK(designated.second->value == 8);

    Graph delegated{};
    CHECK(delegated.first->value == 41);
    CHECK(delegated.second->value == 42);

    Graph valueInitialised{9, true};
    CHECK(valueInitialised.first->value == 9);
    CHECK(valueInitialised.second->value == 0);
}
