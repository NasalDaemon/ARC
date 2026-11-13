#include "arc/tests/test_trunk.hxx"

#include "arc/arc.hpp"

#include <doctest/doctest.h>

// Compiled once as a test, and again with each ARC_TRUNK_* case defined, where the graph it builds must fail the
// compile. Provider's trunks are declared in another namespace, whose trait names the generator does not compare with
// the clients', so the compiler's checks are what reject each case.

#if 0
// arc-begin
#include "arc/arc.hpp"

namespace arc::tests::trunk
{
    trait A { a() const -> int }
    trait B { b() const -> int }
    trait C { c() const -> int }
    trait D { d() const -> int }
    trait AB = A + B
    trait B2 = B
    trait Probe { sum() const -> int }
}
// arc-end

// arc-begin
// Its default trunk is the union of its named trunks: A + B + C + D
cluster arc::tests::trunk::faces::Provider [R = Root]
                                           [Trunk AB = trait::A + trait::B]
                                           [Trunk Extra = trait::C + trait::D]
                                           [Trunk AC = trait::A + trait::C]
                                           [Trunk ABC = Provider::AB + trait::C]
                                           [Trunk ABCD = Provider::ABC + trait::D]
{
    abcd = R::Provider

    [trait::A] .. --> abcd
    [trait::B] .. --> abcd
    [trait::C] .. --> abcd
    [trait::D] .. --> abcd
}
// arc-end

// arc-begin
// Takes Provider's trunk AB from its parent, using all of it
cluster arc::tests::trunk::Middle [R = Root]
{
    inner = R::FullClient

    [trait::Probe] .. --> inner
    [[faces::cluster::Provider::AB]] inner ==> ..
}
// arc-end

// arc-begin
// Takes Provider's trunk AB from its parent, using only trait::A of it
cluster arc::tests::trunk::PartialMiddle [R = Root]
{
    inner = R::PartialClient

    [trait::Probe] .. --> inner
    [[faces::cluster::Provider::AB*]] inner ==> ..
}
// arc-end

// arc-begin
// Takes trunk AB's traits from its parent in one inline trunk, using all of them
cluster arc::tests::trunk::InlineMiddle [R = Root]
{
    inner = R::FullClient

    [trait::Probe] .. --> inner
    [[trait::A + trait::B]] inner ==> ..
}
// arc-end

// arc-begin
// Takes trunk AB's traits from its parent in one inline trunk, using only trait::A of them
cluster arc::tests::trunk::PartialInlineMiddle [R = Root]
{
    inner = R::PartialClient

    [trait::Probe] .. --> inner
    [[trait::A + trait::B]] inner ==> ..
}
// arc-end

// arc-begin
// Takes trunk AB's traits from its parent one link each, using only trait::A of them
cluster arc::tests::trunk::PartialPerTraitMiddle [R = Root]
{
    inner = R::PartialClient

    [trait::Probe] .. --> inner
    [trait::A] inner --> ..
    [trait::B] inner --> ..
}
// arc-end

// arc-begin
// Takes trunk AB's traits from its parent one link each, its node asking for trait::B as trait::B2
cluster arc::tests::trunk::RenameMiddle [R = Root]
{
    inner = R::RenamedClient

    [trait::Probe] .. --> inner
    [trait::A] inner --> ..
    [trait::B] inner (trait::B2) --> ..
}
// arc-end

// arc-begin
// Takes trunk AB's traits from its parent one link each, to a node that lists no dependencies, which counts as using
// all that its links carry: a known gap, as the node may use less
cluster arc::tests::trunk::BareMiddle [R = Root]
{
    inner = R::BareClient

    [trait::Probe] .. --> inner
    [trait::A] inner --> ..
    [trait::B] inner --> ..
}
// arc-end

// arc-begin
// Takes trunk AB's traits from its parent one link each, and passes calls on trait::B on to lis and back out to its
// own parent, as a listener chain does
cluster arc::tests::trunk::SelfForwardMiddle [R = Root]
{
    inner = R::FullClient
    lis   = R::Provider

    [trait::Probe] .. --> inner
    [trait::A] inner --> ..
    [trait::B] inner --> ..
    [trait::B] @parent --> {lis, @parent}
}
// arc-end

// arc-begin
// As SelfForwardMiddle, but its node uses only trait::A: passing trait::B back out is no use of it
cluster arc::tests::trunk::PartialSelfForwardMiddle [R = Root]
{
    inner = R::PartialClient
    lis   = R::Provider

    [trait::Probe] .. --> inner
    [trait::A] inner --> ..
    [trait::B] @parent --> {lis, @parent}
}
// arc-end

// arc-begin
cluster arc::tests::trunk::Faces [R = Root]
{
    provider      = faces::cluster::Provider
    full          = R::FullClient
    optional      = R::OptionalClient
    partial       = R::PartialClient
    extra         = R::ExtraClient
    every         = R::EveryClient
    everything    = R::EveryClient
    middle        = cluster::Middle
    partialMiddle = cluster::PartialMiddle
    clients       = R::FullClient : arc::StaticMap<int>
    lazy          = R::FullClient : arc::Lazy
    either        = arc::Union<typename R::FullClient, typename R::OptionalClient>
    inlineMiddle  = cluster::InlineMiddle
    renameMiddle  = cluster::RenameMiddle
    renamePartial = cluster::RenameMiddle
    bareMiddle    = cluster::BareMiddle
    selfForward   = cluster::SelfForwardMiddle
    narrowed      = R::FullClient : arc::Narrow<trait::Probe>
    combined      = R::FullClient : arc::Combine<typename R::ExtraClient>

    [trait::Probe] .. --> clients

    [[faces::cluster::Provider::AB]]        full, optional, middle, clients, lazy, either, inlineMiddle ==> provider
    [[faces::cluster::Provider::AB]]        renameMiddle, bareMiddle, narrowed, combined, selfForward ==> provider
    [[faces::cluster::Provider::AB*]]       renamePartial ==> provider
    [[faces::cluster::Provider::Extra]]     combined ==> provider
    [[faces::cluster::Provider::AB*]]       partial, partialMiddle ==> provider
    [[faces::cluster::Provider::Extra]] extra ==> provider
    [[faces::cluster::Provider::ABCD]]  every ==> provider
    [[faces::cluster::Provider]]        everything ==> provider
}
// arc-end

// arc-begin
cluster arc::tests::trunk::OverlapExact [R = Root]
{
    provider = faces::cluster::Provider
    other    = R::Provider
    client   = R::FullClient

    [[faces::cluster::Provider::AB]] client ==> provider
    [trait::A] client --> other
}
// arc-end

// arc-begin
cluster arc::tests::trunk::OverlapTrunks [R = Root]
{
    provider = faces::cluster::Provider
    client   = R::FullClient

    [[faces::cluster::Provider::AB]]      client ==> provider
    [[faces::cluster::Provider::AC*]] client ==> provider
}
// arc-end

// arc-begin
cluster arc::tests::trunk::OverlapJoined [R = Root]
{
    provider = faces::cluster::Provider
    joined   = R::JoinedProvider
    client   = R::FullClient

    [[faces::cluster::Provider::AB]] client ==> provider
    [trait::AB] client --> joined
}
// arc-end

// arc-begin
cluster arc::tests::trunk::OverlapFanout [R = Root]
{
    provider = faces::cluster::Provider
    first    = R::Provider
    second   = R::Provider
    client   = R::FullClient

    [[faces::cluster::Provider::AB]] client ==> provider
    [trait::A] client --> {first, second}
}
// arc-end

// arc-begin
cluster arc::tests::trunk::OverlapSink [R = Root]
{
    provider = faces::cluster::Provider
    joined   = R::JoinedProvider
    client   = R::FullClient

    [trait::AB] joined

    [[faces::cluster::Provider::AB]] client ==> provider
}
// arc-end

// arc-begin
cluster arc::tests::trunk::OverlapParent [R = Root]
{
    provider = faces::cluster::Provider
    other    = R::Provider

    [[faces::cluster::Provider::AB]] .. ==> provider
    [trait::A] .. --> other
}
// arc-end

// arc-begin
cluster arc::tests::trunk::PartialNode [R = Root]
{
    provider = faces::cluster::Provider
    client   = R::PartialClient

    [[faces::cluster::Provider::AB]] client ==> provider
}
// arc-end

// arc-begin
cluster arc::tests::trunk::PartialSubcluster [R = Root]
{
    provider = faces::cluster::Provider
    client   = cluster::PartialMiddle

    [[faces::cluster::Provider::AB]] client ==> provider
}
// arc-end

// arc-begin
cluster arc::tests::trunk::PartialInlineSubcluster [R = Root]
{
    provider = faces::cluster::Provider
    client   = cluster::PartialInlineMiddle

    [[faces::cluster::Provider::AB]] client ==> provider
}
// arc-end

// arc-begin
cluster arc::tests::trunk::PartialPerTraitSubcluster [R = Root]
{
    provider = faces::cluster::Provider
    client   = cluster::PartialPerTraitMiddle

    [[faces::cluster::Provider::AB]] client ==> provider
}
// arc-end

// arc-begin
cluster arc::tests::trunk::PartialSelfForwardSubcluster [R = Root]
{
    provider = faces::cluster::Provider
    client   = cluster::PartialSelfForwardMiddle

    [[faces::cluster::Provider::AB]] client ==> provider
}
// arc-end

// arc-begin
cluster arc::tests::trunk::Undeclared [R = Root]
{
    provider = faces::cluster::Provider
    client   = R::BareClient

    [[faces::cluster::Provider::AB]] client ==> provider
}
// arc-end
#endif

namespace arc::tests::trunk {

struct Provider : arc::NodeImpl<trait::A, trait::B, trait::C, trait::D>
{
    int a() const { return 1; }
    int b() const { return 2; }
    int c() const { return 4; }
    int d() const { return 8; }
};

struct JoinedProvider : arc::NodeImpl<trait::AB>
{
    int a() const { return 1; }
    int b() const { return 2; }
};

struct FullClient : arc::Node::Impl<trait::Probe>::Uses<trait::A, trait::B>
{
    int sum(this auto const& self) { return self.getA().a() + self.getB().b(); }
};

// trait::B is optional, and so still counts as used
struct OptionalClient : arc::Node::Impl<trait::Probe>::Uses<trait::A, trait::B*>
{
    int sum(this auto const& self) { return self.getA().a(); }
};

struct PartialClient : arc::Node::Impl<trait::Probe>::Uses<trait::A>
{
    int sum(this auto const& self) { return self.getA().a(); }
};

// Asks for trait::B as trait::B2
struct RenamedClient : arc::Node::Impl<trait::Probe>::Uses<trait::A, trait::B2>
{
    int sum(this auto const& self) { return self.getA().a() + self.getB2().b(); }
};

struct ExtraClient : arc::Node::Impl<trait::Probe>::Uses<trait::C, trait::D>
{
    int sum(this auto const& self) { return self.getC().c() + self.getD().d(); }
};

struct EveryClient : arc::Node::Impl<trait::Probe>::Uses<trait::A, trait::B, trait::C, trait::D>
{
    int sum(this auto const& self) { return self.getA().a() + self.getB().b() + self.getC().c() + self.getD().d(); }
};

// Lists no dependencies: arc::NodeImpl<> alone would list none, so taking a trunk in full would leave it all unused
struct BareClient : arc::Node
{
    using Traits = arc::Traits<trait::Probe>;

    int impl(trait::Probe::sum) const { return 0; }
};

struct Root
{
    using Provider = trunk::Provider;
    using JoinedProvider = trunk::JoinedProvider;
    using FullClient = trunk::FullClient;
    using OptionalClient = trunk::OptionalClient;
    using PartialClient = trunk::PartialClient;
    using ExtraClient = trunk::ExtraClient;
    using RenamedClient = trunk::RenamedClient;
    using EveryClient = trunk::EveryClient;
    using BareClient = trunk::BareClient;
};

// The default trunk of a cluster with named trunks is their union
static_assert(std::same_as<arc::TrunkOf<faces::cluster::Provider>, arc::Trunk<trait::A, trait::B, trait::C, trait::D>>);
static_assert(std::same_as<arc::TrunkOf<faces::cluster::Provider::AB>, arc::Trunk<trait::A, trait::B>>);
static_assert(std::same_as<arc::TrunkOf<faces::cluster::Provider::Extra>, arc::Trunk<trait::C, trait::D>>);
static_assert(std::same_as<arc::TrunkOf<faces::cluster::Provider::AC>, arc::Trunk<trait::A, trait::C>>);
// A trunk that extends another lists the traits of both
static_assert(std::same_as<arc::TrunkOf<faces::cluster::Provider::ABC>, arc::Trunk<trait::A, trait::B, trait::C>>);
static_assert(std::same_as<arc::TrunkOf<faces::cluster::Provider::ABCD>, arc::Trunk<trait::A, trait::B, trait::C, trait::D>>);

} // namespace arc::tests::trunk

using namespace arc::tests::trunk;

TEST_CASE("Clients reach a cluster's traits through its default and named trunks")
{
    arc::Graph<cluster::Faces, Root> graph{.clients{1, [](auto add) { add(0); }}, .either{std::in_place_index<0>}};

    CHECK(graph.full.asTrait(trait::probe).sum() == 3);
    CHECK(graph.optional.asTrait(trait::probe).sum() == 1);
    CHECK(graph.partial.asTrait(trait::probe).sum() == 1);
    CHECK(graph.extra.asTrait(trait::probe).sum() == 12);
    CHECK(graph.every.asTrait(trait::probe).sum() == 15);
    CHECK(graph.everything.asTrait(trait::probe).sum() == 15);
    // Wrapped clients use what the nodes they wrap use
    CHECK(graph.lazy.asTrait(trait::probe).sum() == 3);
    CHECK(graph.either.asTrait(trait::probe).sum() == 3);
    CHECK(graph.inlineMiddle.asTrait(trait::probe).sum() == 3);
    CHECK(graph.renameMiddle.asTrait(trait::probe).sum() == 3);
    CHECK(graph.renamePartial.asTrait(trait::probe).sum() == 3);
    CHECK(graph.bareMiddle.asTrait(trait::probe).sum() == 0);
    CHECK(graph.selfForward.asTrait(trait::probe).sum() == 3);
    CHECK(graph.narrowed.asTrait(trait::probe).sum() == 3);
    // A combined node's later nodes come before its first, so trait::Probe is ExtraClient's
    CHECK(graph.combined.asTrait(trait::probe).sum() == 12);
    CHECK(graph.middle.asTrait(trait::probe).sum() == 3);
    CHECK(graph.partialMiddle.asTrait(trait::probe).sum() == 1);
    CHECK(graph.asTrait(trait::probe, arc::key::Element(0)).sum() == 3);

#if defined(ARC_TRUNK_OVERLAP_EXACT)
    arc::Graph<cluster::OverlapExact, Root> rejected{};
#elif defined(ARC_TRUNK_OVERLAP_TRUNKS)
    arc::Graph<cluster::OverlapTrunks, Root> rejected{};
#elif defined(ARC_TRUNK_OVERLAP_JOINED)
    arc::Graph<cluster::OverlapJoined, Root> rejected{};
#elif defined(ARC_TRUNK_OVERLAP_FANOUT)
    arc::Graph<cluster::OverlapFanout, Root> rejected{};
#elif defined(ARC_TRUNK_OVERLAP_SINK)
    arc::Graph<cluster::OverlapSink, Root> rejected{};
#elif defined(ARC_TRUNK_OVERLAP_PARENT)
    arc::Graph<cluster::OverlapParent, Root> rejected{};
#elif defined(ARC_TRUNK_PARTIAL_NODE)
    arc::Graph<cluster::PartialNode, Root> rejected{};
#elif defined(ARC_TRUNK_PARTIAL_SUBCLUSTER)
    arc::Graph<cluster::PartialSubcluster, Root> rejected{};
#elif defined(ARC_TRUNK_PARTIAL_INLINE_SUBCLUSTER)
    arc::Graph<cluster::PartialInlineSubcluster, Root> rejected{};
#elif defined(ARC_TRUNK_PARTIAL_PER_TRAIT_SUBCLUSTER)
    arc::Graph<cluster::PartialPerTraitSubcluster, Root> rejected{};
#elif defined(ARC_TRUNK_PARTIAL_SELF_FORWARD_SUBCLUSTER)
    arc::Graph<cluster::PartialSelfForwardSubcluster, Root> rejected{};
#elif defined(ARC_TRUNK_UNDECLARED)
    arc::Graph<cluster::Undeclared, Root> rejected{};
#endif
}
