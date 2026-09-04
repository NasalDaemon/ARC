#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

import arc.tests.const_trait;
import arc;

#include "arc/doctest.h"

namespace arc::tests::const_trait {

/* arc-begin

export module arc.tests.const_trait;

namespace arc::tests::const_trait {

// [Const] ensures every method provided by the trait is const — the generator
// rejects any method that is not declared const (including `...` methods).
trait Reader [Const]
{
    get(int i) const -> int
    count() const -> int&
    log(...) const

    // Default impls must also be const; they receive a `const&` self
    half() const -> int { return self.get(0) / 2; }
}

} // namespace arc::tests::const_trait

arc-end */

namespace node {

struct Data : arc::NodeImpl<Reader>
{
    int  impl(Reader::get, int i) const { return i + offset; }
    int& impl(Reader::count) const { return offset; }
    template<class... Args>
    void impl(Reader::log, Args&&...) const { ++logCount; }

    mutable int logCount = 0;
    mutable int offset = 40;
};

} // namespace node

SCENARIO(R"([Const] trait: all methods are callable through a const trait view)")
{
    GIVEN(R"(a node implementing a [Const] trait with all-const methods)")
    {
        arc::test::Graph<node::Data> graph;
        auto const& reader = graph.asTrait(trait::reader);

        WHEN(R"(get(2) is called through the const view)")
        {
            THEN(R"(the const implementation is invoked)")
            {
                CHECK(reader.get(2) == 42);
            }
        }

        WHEN(R"(count() is called through the const view)")
        {
            THEN(R"(a mutable reference is still returned by the const method)")
            {
                CHECK(reader.count() == 40);
                reader.count() = 7;
                CHECK(graph.asTrait(trait::reader).count() == 7);
            }
        }

        WHEN(R"(log(...) is called with variadic arguments through the const view)")
        {
            reader.log("x", 1, 2.0);
            reader.log();

            THEN(R"(the const variadic implementation is invoked for each call)")
            {
                CHECK(graph.node->logCount == 2);
            }
        }

        WHEN(R"(the defaulted half() is called through the const view)")
        {
            THEN(R"(the const default impl dispatches to the const self.get())")
            {
                CHECK(reader.half() == 20);
            }
        }
    }
}

} // namespace arc::tests::const_trait
