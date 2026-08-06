---
name: arc-testing
description: ARC testing guide using doctest and BDD macros with ARC testing framework
user-invocable: true
---

# Testing

Uses [doctest](https://github.com/doctest/doctest) with BDD macros: `SCENARIO`, `GIVEN`, `WHEN`, `THEN`, `AND_WHEN`, `AND_THEN`. For CMake test setup see `/arc-cmake`. For the spy builtin used in integration tests see `/arc-node-builtins`.

## General BDD Principles

1. Behaviour is tested, not implementation details.
    - Focus on observable inputs and outputs of trait methods, never internal state or private methods.
2. Only access a node's internal state during test initialisation (setup), never for behaviour verification.
    - Unobservable behaviour is a signal the trait design needs improvement.
3. Exercise nodes under test exclusively through their trait methods.
    - Mocks and test doubles are exempt: they exist to make interactions observable and controllable.
4. Use `GIVEN` for setup, `WHEN` for actions, and `THEN` for assertions.
    - Avoid mixing setup and assertions in the same block to maintain clarity.
    - Use `R"(Text)"` raw string literals in test descriptions instead of escaping for legibility.
5. Use `AND_WHEN`/`AND_THEN` only when nested inside respective `WHEN`/`THEN` blocks.
    - Sibling `AND_WHEN`/`AND_THEN` blocks are mutually independent, running in parallel.
    - Nested `AND_WHEN`/`AND_THEN` blocks are sequential steps that depend on the parent block.

## Unit Test (single node, mocked dependencies)

```cpp
#include <doctest/doctest.h>
import app.tests.graphs;
import app.traits;
import arc;

SCENARIO(R"(MyNode does something "useful")")
{
    GIVEN(R"(a precondition)")
    {
        arc::test::Graph<MyNode> graph;
        graph.mocks->define(
            [](trait::Dep::method, int arg) { return arg * 2; },
            [](trait::Dep::otherMethod) -> std::string { return "mocked"; }
        );
        auto node = graph.asTrait(trait::myTrait);

        WHEN(R"(an action is performed)")
        {
            auto result = node.doSomething();

            THEN(R"(the expected outcome holds)")
            {
                CHECK(result == expected);
            }

            AND_WHEN(R"(another action is performed on top)")
            {
                auto other = node.doSomethingElse();

                THEN(R"(another expected outcome holds)")
                {
                    CHECK(node.otherThing() == other);

                    AND_THEN(R"(a nested assertion)")
                    {
                        CHECK(node.nestedThing() == 42);
                    }
                }
            }
        }
    }
}
```

## Unit Test with Custom Test Double

Instead of using the generic mock, you can provide a concrete test double node, or combine multiple using `arc::Combine`:

```cpp
struct AuthServiceTestDouble : arc::test::TestOnlyNode
{
    using Traits = arc::Traits<trait::AuthService>;

    // Only implement the methods actually called by the node under test
    bool impl(this auto& self, trait::AuthService::logIn, std::string_view user, std::string_view pass)
    {
        return self.users.contains(user) && self.users[user] == pass;
    }

    std::map<std::string, std::string> users;
};

TEST_CASE(R"(With a single test double)")
{
    arc::test::Graph<node::Sessions, AuthServiceTestDouble> graph;
    graph.mocks->addUser("alice", "pass123");
    // ...
}
TEST_CASE(R"(With multiple test doubles)")
{
    arc::test::Graph<node::Sessions, arc::Combine<MockLogger, AuthServiceTestDouble>> graph;
    graph.mocks->addUser("alice", "pass123");
    // ...
}
```

`arc::test::TestOnlyNode` marks nodes that can only be used in test graphs.

## Mock API Reference

### Construction

`MockParams` configures initial behaviour:
```cpp
arc::test::Graph<MyNode> graph{.mocks{test::MockParams{
    .defaultBehaviour = test::MockDefault::ThrowIfMissing,  // or ReturnDefault
    .counting = false,
    .logAllCalls = false,
    .logBufferMaxSize = 1024,
}}};
```

### Default Behaviour

What happens when a mock method is called without a definition:
```cpp
graph.mocks->setThrowIfMissing();  // Throw (good for catching missing mocks)
graph.mocks->setReturnDefault();   // Return default-constructed value (T{})
```
Prefer `ThrowIfMissing` during test development to catch missing mocks, then switch to `ReturnDefault` if you want to allow calls without explicit definitions (e.g. for a large number of unused methods).

### Defining Mock Implementations

Lambdas take `(trait::T::method, args...)`:
```cpp
// Define for both const and mutable calls
graph.mocks->define(
    [](trait::Logger::log, std::string_view msg) {},          // void return
    [](trait::Calculator::add, int a, int b) { return a+b; }, // value return
    [&](trait::Store::ref) -> int& { return myInt; }          // reference return
);

// Const-only or mutable-only definitions
graph.mocks->defineConst([](trait::T::method) { return 1; });
graph.mocks->defineMut([](trait::T::method) { return 2; });

// Fixed return value without a lambda (lower priority than define/defineConst/defineMut)
graph.mocks->implReturns<trait::Calculator::add, int, int>(42);
// Fixed return value for all method overloads (lower priority than implReturns)
graph.mocks->methodReturns<trait::Calculator::add>(42);

// Sequence of return values (returned in order, throws when exhausted, return type optionally specified in second template param)
graph.mocks->methodReturnsN<trait::LineReader::readLine, std::optional<std::string>>("first", "second", std::nullopt);
graph.mocks->implReturnsN<trait::LineReader::readLine(std::string_view), std::optional<std::string>>("first", std::nullopt);

// Remove all matching definitions and return values of impl
graph.mocks->undefine<trait::Calculator::add, int, int>();
// Remove matching methodReturns definition
graph.mocks->undefineMethodReturns<trait::Calculator::add>();
```

### Call Counting

Track how many times methods/traits are called:
```cpp
graph.mocks->enableCallCounting();

// After exercising the node...
graph.mocks->totalCallCount();                          // All calls
graph.mocks->traitCallCount<trait::Logger>();            // All Logger calls
graph.mocks->methodCallCount<trait::Logger::log>();      // Specific method
graph.mocks->implCallCount<trait::T::method, int>();     // Specific overload
```

### Call Logging

Inspect actual arguments passed to mocks:
```cpp
graph.mocks->logAllCalls();

// After exercising the node...
auto visitor = graph.mocks->visitCallLogs<trait::T::method, int>();

visitor.size();                       // Number of unvisited calls
visitor.popFront();                   // Returns optional<tuple<args...>>, advances
visitor.back();                       // Last call (doesn't advance)
visitor.currentIndex();               // Index of next call to visit
visitor.lastVisitedIndex();           // Index of last popFront/back result
visitor.findNext(std::tuple(42));     // Find next call with matching args
visitor.findLast(std::tuple(42));     // Find last call with matching args
visitor.restart();                    // Re-read from the beginning
visitor.restartFrom(index);           // Re-read from the specified index
visitor.rebind();                     // Must be called if graph.mocks->resetTracking() is called
```

### Reset

Clear state between test phases:
```cpp
graph.mocks->resetTracking();         // Clear counters and logs
graph.mocks->resetImpls();            // Clear all definitions and return values
graph.mocks->resetTrackingAndImpls(); // Clear both
graph.mocks->reinitialise();          // Full reset to fresh MockParams
```

## Integration Test (full cluster, real nodes)

```cpp
// Strategically swap out nodes to test doubles if needed using a custom root
struct IntegrationTestRoot
{
    using Logger = test::node::MockLogger;  // Provide real implementations for all Logger methods
};

SCENARIO(R"(App cluster end-to-end with mock logger)")
{
    arc::Graph<cluster::App, IntegrationTestRoot> graph;
    auto myTrait = graph.myNode.asTrait(trait::myTrait);

    GIVEN(R"(a fresh cluster)")
    {
        WHEN(R"(an action is performed)")
        {
            THEN(R"(the expected outcome holds)") { /* assertions */ }
        }
    }
}

struct IntegrationSpy : arc::NodeImpl<arc::trait::Spy>
{
    template<class Method, class... Args>
    decltype(auto) intercept(Method, auto impl_fn, Args&&... args)
    {
        ++callCount;
        return impl_fn(std::forward<Args>(args)...);
    }
    int callCount = 0;
};

SCENARIO(R"(App cluster end-to-end with spying)")
{
    arc::GraphWithGlobal<cluster::App, IntegrationSpy> graph;
    auto spy = graph.global.asTrait(arc::trait::Spy);
    auto myTrait = graph.main.myNode.asTrait(trait::myTrait);

    GIVEN(R"(a fresh cluster)")
    {
        WHEN(R"(an action is performed)")
        {
            THEN(R"(the expected outcome holds)") { /* assertions */ }
        }
    }
}
```

## Test Graph Type Aliases

Define in `tests/graphs.ixx`:
```cpp
export module app.tests.graphs;
import app.node.my_node;
import arc;

namespace app::tests {
export using MyNodeTestGraph = arc::test::Graph<node::MyNode>;
}
```

## Test-Only Clusters (Embedded DSL)

For testing a subset of nodes wired together, embed ARC DSL directly in the test file using `arc-begin`/`arc-end` markers. Multiple sections can be spread across the file and are stitched together:

```cpp
#include <doctest/doctest.h>

import app.tests.my_test;

/* arc-begin
export module app.tests.my_test;
import app.traits;

cluster app::tests::my_test::TestCluster
{
    nodeA = node::A
    nodeB = node::B
    [Trait] nodeA --> nodeB
}
arc-end */

namespace app::tests::my_test {

TEST_CASE(R"(first test)") { /* ... */ }

/* arc-begin
cluster app::tests::my_test::TestCluster2 [Root]
{
    nodeDefinedInTest = Root::TestNode
    // second cluster in same file
}
arc-end */

struct Root
{
    struct TestNode : arc::NodeImpl<trait::T>
    {
        int method() { return 42; }
    };
};

TEST_CASE(R"(second test)")
{
    arc::test::Graph<TestCluster2, arc::test::Mock<>, Root> graph;
    // ...
}

}
```

Then use `EMBED` in CMakeLists.txt:
```cmake
target_generate_arc_modules(app_tests EMBED tests/test_my_test.cpp)
```
Remember to generate any impl srcs using the test graph type. See `/arc-cmake` skill for further details if necessary.
