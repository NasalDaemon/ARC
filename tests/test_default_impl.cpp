#include "arc/macros.hpp"

#if !ARC_IMPORT_STD
#include <type_traits>
#endif

import arc.tests.default_impl;
import arc;

#include "arc/doctest.h"

namespace arc::tests::default_impl {

/* arc-begin

export module arc.tests.default_impl;

namespace arc::tests::default_impl {

trait Counter
{
    increment()
    decrement()
    reset() {}
    count() const -> int
    {
        self.reset();
        return 0;
    }
    add(int n) {}
    sub(int n) {}
}

} // namespace arc::tests::default_impl

arc-end */

namespace node {

// Only implements the two non-defaulted methods; count/reset/add/sub use defaults.
struct MinimalCounter : arc::Node
{
    using Traits = arc::Traits<Counter>;
    void impl(Counter::increment) {}
    void impl(Counter::decrement) {}
};

// Overrides all methods so the default impls are never reached.
struct FullCounter : arc::NodeImpl<Counter>
{
    void increment() { ++value; }
    void decrement() { --value; }
    void reset()     { value = 0; }
    int  count() const { return value; }
    void add(int n)  { value += n; }
    void sub(int n)  { value -= n; }

    int value = 0;
};

// Overrides only count() to observe calls; leaves mutating defaults as no-ops.
struct ReadOnlyCounter : arc::NodeImpl<Counter>
{
    void increment() {}
    void decrement() {}
    int  count() const { return value; }

    int value = 42;
};

// Calls defaulted trait methods (reset, add) from within its own non-default impls
// using the impl-qualified style (NodeImpl<Counter*> + impl(Method) overloads).
struct CallsDefaultInImplQualified : arc::NodeImpl<Counter*>
{
    void impl(this auto& self, Counter::increment)
    {
        self.reset(); ++self.value;
    }
    void impl(this auto& self, Counter::decrement)
    {
        self.add(-1); --self.value;
    }
    int impl(Counter::count) const { return value; }

    int value = 0;
};

// Same behaviour, but using the named-method style (NodeImpl<Counter> + named
// member fns), dispatching defaulted calls via self.asTrait(...).method().
struct CallsDefaultInImplNamed : arc::NodeImpl<Counter>
{
    void increment(this auto& self)
    {
        self.asCounter().reset(); ++self.value;
    }
    void decrement(this auto& self)
    {
        // Test internal method dispatch for correct default impl behaviour
        self.Methods::add(-1); --self.value;
    }
    int count() const { return value; }

    int value = 0;
};

// Spy that counts intercepts per Counter method.
struct CounterSpy : arc::NodeImpl<arc::Spy>
{
    template<class Method>
    decltype(auto) intercept(this auto& self, Method, auto f, auto&&... args)
    {
        ++self.totalCount;
        if constexpr (std::is_same_v<Method, Counter::reset>)
            ++self.resetCount;
        else if constexpr (std::is_same_v<Method, Counter::count>)
            ++self.countCount;
        else if constexpr (std::is_same_v<Method, Counter::add>)
            ++self.addCount;
        else if constexpr (std::is_same_v<Method, Counter::sub>)
            ++self.subCount;
        else if constexpr (std::is_same_v<Method, Counter::increment>)
            ++self.incrementCount;
        else if constexpr (std::is_same_v<Method, Counter::decrement>)
            ++self.decrementCount;
        return f(ARC_FWD(args)...);
    }

    mutable int totalCount = 0;
    mutable int resetCount = 0;
    mutable int countCount = 0;
    mutable int addCount = 0;
    mutable int subCount = 0;
    mutable int incrementCount = 0;
    mutable int decrementCount = 0;
};

} // namespace node

SCENARIO(R"(Counter default impls: unoverridden methods use the declared default behaviour)")
{
    GIVEN(R"(a counter that only implements increment and decrement)")
    {
        arc::test::Graph<node::MinimalCounter> graph;
        auto counter = graph.asTrait(trait::counter);

        WHEN(R"(count() is queried without any prior calls)")
        {
            THEN(R"(the default 0 is returned)")
            {
                CHECK(counter.count() == 0);
            }
        }

        WHEN(R"(reset() is called)")
        {
            counter.reset();

            THEN(R"(it is a no-op and count() still returns 0)")
            {
                CHECK(counter.count() == 0);
            }
        }

        WHEN(R"(add(5) is called)")
        {
            counter.add(5);

            THEN(R"(it is a no-op and count() still returns 0)")
            {
                CHECK(counter.count() == 0);
            }
        }

        WHEN(R"(sub(3) is called)")
        {
            counter.sub(3);

            THEN(R"(it is a no-op and count() still returns 0)")
            {
                CHECK(counter.count() == 0);
            }
        }
    }
}

SCENARIO(R"(Counter default impls: overriding all defaults replaces each default behaviour)")
{
    GIVEN(R"(a counter that overrides all methods)")
    {
        arc::test::Graph<node::FullCounter> graph;
        auto counter = graph.asTrait(trait::counter);

        WHEN(R"(increment() is called)")
        {
            counter.increment();

            THEN(R"(count() returns 1)")
            {
                CHECK(counter.count() == 1);
            }
        }

        WHEN(R"(add(5) is called)")
        {
            counter.add(5);

            THEN(R"(count() returns 5)")
            {
                CHECK(counter.count() == 5);
            }
        }

        WHEN(R"(sub(2) is called after add(5))")
        {
            counter.add(5);
            counter.sub(2);

            THEN(R"(count() reflects the net change)")
            {
                CHECK(counter.count() == 3);
            }
        }

        WHEN(R"(state is accumulated and then reset)")
        {
            counter.add(10);
            counter.increment();

            AND_WHEN(R"(reset() is called)")
            {
                counter.reset();

                THEN(R"(count() returns 0)")
                {
                    CHECK(counter.count() == 0);
                }
            }
        }
    }
}

SCENARIO(R"(Counter default impls: partial override — only count() is overridden)")
{
    GIVEN(R"(a counter that overrides only count() with a fixed value of 42)")
    {
        arc::test::Graph<node::ReadOnlyCounter> graph;
        auto counter = graph.asTrait(trait::counter);

        WHEN(R"(count() is queried)")
        {
            THEN(R"(the overridden value 42 is returned, not the default 0)")
            {
                CHECK(counter.count() == 42);
            }
        }

        WHEN(R"(add(5) is called)")
        {
            counter.add(5);

            THEN(R"(add() is still a no-op and count() remains 42)")
            {
                CHECK(counter.count() == 42);
            }
        }

        WHEN(R"(reset() is called)")
        {
            counter.reset();

            THEN(R"(reset() is still a no-op and count() remains 42)")
            {
                CHECK(counter.count() == 42);
            }
        }
    }
}

SCENARIO_TEMPLATE(R"(Counter default impls: a node impl may call a default method on self)", Node,
                  node::CallsDefaultInImplNamed,
                  node::CallsDefaultInImplQualified)
{
    GIVEN(R"(a counter whose increment/decrement call defaulted trait methods on self)")
    {
        arc::test::Graph<Node> graph;
        auto counter = graph.asTrait(trait::counter);

        WHEN(R"(increment() is called twice (each invokes the defaulted self.reset()))")
        {
            counter.increment();
            counter.increment();

            THEN(R"(the defaulted reset is a no-op and only the impl's own ++value is observed)")
            {
                CHECK(counter.count() == 2);
            }
        }

        WHEN(R"(decrement() is called (invokes the defaulted self.add(-1)))")
        {
            counter.decrement();

            THEN(R"(the defaulted add is a no-op and only the impl's own --value is observed)")
            {
                CHECK(counter.count() == -1);
            }
        }
    }
}

SCENARIO(R"(Counter default impls: arc::trait::Spy intercepts default method calls)")
{
    GIVEN(R"(a MinimalCounter sitting behind a CounterSpy global)")
    {
        arc::test::GraphWithGlobal<node::MinimalCounter, node::CounterSpy> graph;
        auto counter = graph.asTrait(trait::counter);

        WHEN(R"(add(5) is called on the trait (resolves to defaulted no-op))")
        {
            counter.add(5);

            THEN(R"(spy intercepts exactly the defaulted add())")
            {
                CHECK(graph.global->addCount == 1);
                CHECK(graph.global->totalCount == 1);
            }
        }

        WHEN(R"(sub(3) is called on the trait (resolves to defaulted no-op))")
        {
            counter.sub(3);

            THEN(R"(spy intercepts exactly the defaulted sub())")
            {
                CHECK(graph.global->subCount == 1);
                CHECK(graph.global->totalCount == 1);
            }
        }

        WHEN(R"(count() is called, whose default body calls self.reset())")
        {
            auto value = counter.count();

            THEN(R"(spy intercepts both the outer count() and the inner self.reset())")
            {
                CHECK(value == 0);
                CHECK(graph.global->countCount == 1);
                CHECK(graph.global->resetCount == 1);
                CHECK(graph.global->totalCount == 2);
            }
        }

        WHEN(R"(increment() is called (non-defaulted user impl))")
        {
            counter.increment();

            THEN(R"(spy intercepts the user-provided increment() and nothing else)")
            {
                CHECK(graph.global->incrementCount == 1);
                CHECK(graph.global->totalCount == 1);
            }
        }
    }

}

SCENARIO_TEMPLATE(R"(Counter default impls: spy intercepts default calls reached from inside a node impl)", Node,
                  node::CallsDefaultInImplNamed,
                  node::CallsDefaultInImplQualified)
{
    GIVEN(R"(a CallsDefaultInImpl counter behind a CounterSpy global)")
    {
        arc::test::GraphWithGlobal<Node, node::CounterSpy> graph;
        auto counter = graph.asTrait(trait::counter);

        WHEN(R"(increment() is called, whose impl calls self.reset() (default))")
        {
            counter.increment();

            THEN(R"(spy intercepts both the user increment() and the inner defaulted reset())")
            {
                CHECK(graph.global->incrementCount == 1);
                CHECK(graph.global->resetCount == 1);
            }

            AND_WHEN(R"(count() is queried, dispatched through the overridden count())")
            {
                auto value = counter.count();

                THEN(R"(spy intercepts the overridden count() and value reflects the increment)")
                {
                    CHECK(value == 1);
                    CHECK(graph.global->countCount == 1);
                }
            }
        }

        WHEN(R"(decrement() is called, whose impl calls self.add(-1) (default))")
        {
            counter.decrement();

            THEN(R"(spy intercepts both the user decrement() and the inner defaulted add())")
            {
                CHECK(graph.global->decrementCount == 1);
                CHECK(graph.global->addCount == 1);
                CHECK(counter.count() == -1);
            }
        }
    }
}

} // namespace arc::tests::default_impl
