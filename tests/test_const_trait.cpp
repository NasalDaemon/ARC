#include "arc/doctest_prelude.h"
#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#include <utility>
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

trait Cell
{
    value() -> int&
    value() const -> int const&
    peek() -> int
}

trait Slots [Types]
{
    type Key

    at(Types::Key const& key) -> int&
    at(Types::Key const& key) const -> int const&
    slot(Types::Key const& key) -> int& { return self.at(key); }
    slot(Types::Key const& key) const -> int const& { return self.at(key); }

    template<class F>
    each(Types::Key const& key, F&& f) -> int
    template<class F>
    each(Types::Key const& key, F&& f) const -> int
}

cluster Cells [Root]
{
    cell = Root::Cell
    slots = Root::Slots
    reader = Root::Reader

    [Cell]
    reader --> cell

    [Slots]
    reader --> slots
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

struct Root
{
    struct Cell : arc::NodeImpl<trait::Cell>
    {
        int& value() { return v; }
        int const& value() const { return v; }
        int peek() const { return v; }

        int v = 1;
    };

    struct Slots : arc::NodeImpl<trait::Slots>
    {
        struct Types
        {
            using Key = int;
        };

        int& at(int key) { return v[key]; }
        int const& at(int key) const { return v[key]; }
        int each(int key, auto&& f) { return f(v[key]); }
        int each(int key, auto&& f) const { return f(v[key]) + 100; }

        int v[2] = {1, 2};
    };

    struct Reader : arc::NodeUses<trait::Cell, trait::Slots>
    {
        decltype(auto) read(this auto const& self) { return self.getCell().value(); }
        decltype(auto) write(this auto& self) { return self.getCell().value(); }

        decltype(auto) readAt(this auto const& self, int key) { return self.getSlots().at(key); }
        decltype(auto) writeAt(this auto& self, int key) { return self.getSlots().at(key); }
        decltype(auto) readSlot(this auto const& self, int key) { return self.getSlots().slot(key); }
        decltype(auto) writeSlot(this auto& self, int key) { return self.getSlots().slot(key); }
        int readEach(this auto const& self, int key) { return self.getSlots().each(key, [](int x) { return x; }); }
        int writeEach(this auto& self, int key) { return self.getSlots().each(key, [](int x) { return x; }); }
    };
};

template<class View>
concept CanPeek = requires (View view) { view.peek(); };
template<class View>
concept CanPeekAsFunctor = requires (View view) { view.peek(arc::asFunctor)(); };
template<class View>
concept CanPeekByTag = requires (View view) { view.impl(trait::Cell::peek{}); };

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

SCENARIO(R"(A const node reaches its dependency through the trait's const signatures)")
{
    GIVEN(R"(a reader depending on a cell whose trait has const and non-const overloads)")
    {
        arc::Graph<cluster::Cells, Root> graph;

        THEN(R"(the reader's const method gets the const overload, its non-const method the non-const one)")
        {
            static_assert(std::is_same_v<decltype(graph.reader->read()), int const&>);
            static_assert(std::is_same_v<decltype(graph.reader->write()), int&>);
            graph.reader->write() = 7;
            CHECK(graph.reader->read() == 7);
        }

        THEN(R"(a method the trait declares only non-const is not reached through the const view, even though the cell implements it const)")
        {
            using ConstView = decltype(std::as_const(graph).reader->getCell());
            using View = decltype(graph.reader->getCell());
            static_assert(not CanPeek<ConstView>);
            static_assert(CanPeek<View>);
            static_assert(not CanPeekAsFunctor<ConstView>);
            static_assert(CanPeekAsFunctor<View>);
            static_assert(not CanPeekByTag<ConstView>);
            static_assert(CanPeekByTag<View>);
            using Functor = decltype(std::declval<View&>().peek(arc::asFunctor));
            static_assert(std::is_copy_assignable_v<Functor> and std::is_move_assignable_v<Functor>);
            static_assert(std::is_invocable_v<Functor const&>);
            auto const functor = graph.reader->getCell().peek(arc::asFunctor);
            CHECK(functor() == 1);
            CHECK(graph.reader->getCell().peek() == 1);
        }
    }

    GIVEN(R"(a reader depending on overloads whose parameters name the trait's Types)")
    {
        arc::Graph<cluster::Cells, Root> graph;

        THEN(R"(the reader's const method gets the const overload, its non-const method the non-const one)")
        {
            static_assert(std::is_same_v<decltype(graph.reader->readAt(0)), int const&>);
            static_assert(std::is_same_v<decltype(graph.reader->writeAt(0)), int&>);
            graph.reader->writeAt(1) = 7;
            CHECK(graph.reader->readAt(1) == 7);
        }

        THEN(R"(so do their default implementations)")
        {
            static_assert(std::is_same_v<decltype(graph.reader->readSlot(0)), int const&>);
            static_assert(std::is_same_v<decltype(graph.reader->writeSlot(0)), int&>);
            graph.reader->writeSlot(0) = 5;
            CHECK(graph.reader->readSlot(0) == 5);
        }

        THEN(R"(so do template methods)")
        {
            CHECK(graph.reader->readEach(1) == 102);
            CHECK(graph.reader->writeEach(1) == 2);
        }
    }
}

} // namespace arc::tests::const_trait
