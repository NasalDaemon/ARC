#include "arc/doctest_prelude.h"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <cstdint>
#endif

import arc;
import arc.tests.datastore.layout;

#include "arc/doctest.h"

/* arc-begin
export module arc.tests.datastore.layout;

namespace arc::tests::datastore::layout {

cluster Cluster [R = Root]
{
    store = R::Store
    listener = R::Listener

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    store <-> listener
}

cluster NineListeners [R = Root]
{
    store = R::Store
    l0 = R::Listener
    l1 = R::Listener
    l2 = R::Listener
    l3 = R::Listener
    l4 = R::Listener
    l5 = R::Listener
    l6 = R::Listener
    l7 = R::Listener
    l8 = R::Listener

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    store <-> {l0, l1, l2, l3, l4, l5, l6, l7, l8}
}

cluster SixteenListeners [R = Root]
{
    store = R::Store
    l0 = R::Listener
    l1 = R::Listener
    l2 = R::Listener
    l3 = R::Listener
    l4 = R::Listener
    l5 = R::Listener
    l6 = R::Listener
    l7 = R::Listener
    l8 = R::Listener
    l9 = R::Listener
    l10 = R::Listener
    l11 = R::Listener
    l12 = R::Listener
    l13 = R::Listener
    l14 = R::Listener
    l15 = R::Listener

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    store <-> {l0, l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12, l13, l14, l15}
}

cluster ThirtyThreeListeners [R = Root]
{
    store = R::Store
    l0 = R::Listener
    l1 = R::Listener
    l2 = R::Listener
    l3 = R::Listener
    l4 = R::Listener
    l5 = R::Listener
    l6 = R::Listener
    l7 = R::Listener
    l8 = R::Listener
    l9 = R::Listener
    l10 = R::Listener
    l11 = R::Listener
    l12 = R::Listener
    l13 = R::Listener
    l14 = R::Listener
    l15 = R::Listener
    l16 = R::Listener
    l17 = R::Listener
    l18 = R::Listener
    l19 = R::Listener
    l20 = R::Listener
    l21 = R::Listener
    l22 = R::Listener
    l23 = R::Listener
    l24 = R::Listener
    l25 = R::Listener
    l26 = R::Listener
    l27 = R::Listener
    l28 = R::Listener
    l29 = R::Listener
    l30 = R::Listener
    l31 = R::Listener
    l32 = R::Listener

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    store <-> {l0, l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12, l13, l14, l15, l16, l17, l18, l19, l20, l21, l22, l23, l24, l25, l26, l27, l28, l29, l30, l31, l32}
}

cluster SixtyFourListeners [R = Root]
{
    store = R::Store
    l0 = R::Listener
    l1 = R::Listener
    l2 = R::Listener
    l3 = R::Listener
    l4 = R::Listener
    l5 = R::Listener
    l6 = R::Listener
    l7 = R::Listener
    l8 = R::Listener
    l9 = R::Listener
    l10 = R::Listener
    l11 = R::Listener
    l12 = R::Listener
    l13 = R::Listener
    l14 = R::Listener
    l15 = R::Listener
    l16 = R::Listener
    l17 = R::Listener
    l18 = R::Listener
    l19 = R::Listener
    l20 = R::Listener
    l21 = R::Listener
    l22 = R::Listener
    l23 = R::Listener
    l24 = R::Listener
    l25 = R::Listener
    l26 = R::Listener
    l27 = R::Listener
    l28 = R::Listener
    l29 = R::Listener
    l30 = R::Listener
    l31 = R::Listener
    l32 = R::Listener
    l33 = R::Listener
    l34 = R::Listener
    l35 = R::Listener
    l36 = R::Listener
    l37 = R::Listener
    l38 = R::Listener
    l39 = R::Listener
    l40 = R::Listener
    l41 = R::Listener
    l42 = R::Listener
    l43 = R::Listener
    l44 = R::Listener
    l45 = R::Listener
    l46 = R::Listener
    l47 = R::Listener
    l48 = R::Listener
    l49 = R::Listener
    l50 = R::Listener
    l51 = R::Listener
    l52 = R::Listener
    l53 = R::Listener
    l54 = R::Listener
    l55 = R::Listener
    l56 = R::Listener
    l57 = R::Listener
    l58 = R::Listener
    l59 = R::Listener
    l60 = R::Listener
    l61 = R::Listener
    l62 = R::Listener
    l63 = R::Listener

    [arc::DataStore<typename R::Data> <-> arc::DataListener<typename R::Data>]
    store <-> {l0, l1, l2, l3, l4, l5, l6, l7, l8, l9, l10, l11, l12, l13, l14, l15, l16, l17, l18, l19, l20, l21, l22, l23, l24, l25, l26, l27, l28, l29, l30, l31, l32, l33, l34, l35, l36, l37, l38, l39, l40, l41, l42, l43, l44, l45, l46, l47, l48, l49, l50, l51, l52, l53, l54, l55, l56, l57, l58, l59, l60, l61, l62, l63}
}

}
arc-end */

namespace arc::tests::datastore::layout {

// Each store asserts its slot size against its Body's size and alignment when instantiated

struct Tiny
{
    using Id = char;
    using Value = char;
    Value value;
};

struct Wide
{
    using Id = std::uint64_t;
    using Value = std::uint64_t;
    Value value;
    std::uint64_t rest[2]{};
};

struct alignas(64) Over
{
    using Id = int;
    using Value = int;
    Value value;
};

struct Odd
{
    char bytes[5];
};

template<class D, class P>
struct Root
{
    using Data = D;
    using Store = arc::node::DataStore<D>;

    struct Listener
    {
        template<class Context>
        struct Node : arc::Node::Impl<arc::trait::DataListener<D>>::template Uses<arc::trait::DataStore<D>>
        {
            struct Types
            {
                using PrivateData = P;
            };

            auto init(typename D::Id const&, auto const&, D&, auto) -> P { return {}; }
            void onEvent(auto, auto) {}
        };
    };
};

TEST_CASE_TEMPLATE("arc::DataStore slot layout", R,
    Root<Tiny, arc::NoPrivateData>, Root<Tiny, Odd>, Root<Wide, Odd>, Root<Over, arc::NoPrivateData>, Root<Wide, Over>)
{
    arc::Graph<cluster::Cluster, R> graph;
    REQUIRE(graph.store->add(1, typename R::Data::Value{7}).inserted);
    CHECK(graph.store->get(1)->value == 7);
}

template<class Cluster, class R>
void checkStore()
{
    arc::Graph<Cluster, R> graph;
    REQUIRE(graph.store->add(1, typename R::Data::Value{7}).inserted);
    CHECK(graph.store->get(1)->value == 7);
}

TEST_CASE_TEMPLATE("arc::DataStore slot layout with more than 8 listeners", R, Root<Tiny, arc::NoPrivateData>, Root<Tiny, Odd>, Root<Over, arc::NoPrivateData>)
{
    checkStore<cluster::NineListeners, R>();
    checkStore<cluster::SixteenListeners, R>();
    checkStore<cluster::ThirtyThreeListeners, R>();
    checkStore<cluster::SixtyFourListeners, R>();
}

} // namespace arc::tests::datastore::layout
