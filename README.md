# ARC: Architecture Realised through Code

**ARC** is a C++23 framework that makes architectural intent enforceable: explicit structure, zero-overhead dispatch, independently compiled modules, and a path from modular monolith to microservices. It is a standalone library, importable as a C++20 module or header-only.

## How It Works

ARC structures applications using three core concepts:

1. **Traits** - Interfaces that define contracts between components (e.g., `Greeter`, `Responder`)
2. **Nodes** - Self-contained components that implement traits and declare which traits they depend on
3. **Clusters** - Compositions that wire nodes together, satisfying all dependencies at compile time

The result is a **Graph**: a single, efficient object containing all your nodes with their dependencies resolved. The compiler can inline across node boundaries, giving you modularity without overhead.

These three concepts are enough for a complete app. In keeping with the C++ ethos of "don't pay for what you don't use," all other ARC features (policies, protocols, domains, threading, runtime polymorphism) are orthogonal and stay out of your code until you use them.

## Spec-Driven Development, baked in

The intent of an ARC codebase lives in its clusters, traits and behavioural tests, which makes the node implementations reproducible. Delete every node implementation, but keep those three, and the implementations can be rewritten until it compiles against the contracts and passes the tests. This isn't hypothetical: it was done with the [calculator example](examples/calculator).

This inverts the usual pattern of decay. Normally the architecture erodes: hidden coupling accumulates, the design drifts from what was intended, and the implementations ossify. In ARC the architecture is kept honest by the compiler, so it endures, while a node's functional implementation is replaceable. What lasts is the small and information-dense spec: the traits, the wiring and the tests.

The same trait boundaries that makes an ordinary node's body reproducible also guards the few that aren't. Where a node holds *non-functional* value (latency, allocation, memory layout), its traits seal it [inside that node](docs/node-structure.md). Its value stays concentrated and protected instead of smeared across the codebase where it would either rot or get in the way.

## Why ARC?

- **Architecture as First-Class Code:** Define your system as composable nodes with explicit dependencies and interfaces. Your boundaries are type-safe and compiler-enforced—not documentation that drifts out of sync.
- **Zero overhead:** No cost at graph construction, trait resolution, or method invocation. [Here](benchmarks/compilation/99) is a 99-node graph collapsed into a single line of assembly.
- **No legibility tax:** Static-dispatch in C++ normally means templates and CRTP, which go viral and drag implementations into headers. Not with ARC: calls are direct and inlined (no vtables, heap allocations, or lookups), while remaining clear and concise without noisy template incantations.
- **Fast, parallelised builds:** Each node can compile independently (even when building with C++ modules), so you never pay for modularity with slow builds.
- **Testing is the cheap path:** A mock is just another node, so substitution is compile-time-safe and zero-cost in production. Wide coverage becomes the path of least resistance instead of a discipline you must enforce.
- **Group Access Policy Control:** Enforce strict segregation between nodes at compile time. Suited to safety-critical standards where architectural boundaries must be guaranteed with zero runtime overhead.
- **Protocol Contracts:** Attach a protocol state machine to any trait to enforce legal state transitions and invariants at every call site—catching violations at the boundary, not as silent downstream corruption. Elided entirely in release builds by default.
- **Thread Safety by Design:** Thread affinity of nodes can be enforced at compile time, eliminating entire classes of concurrency bugs before they start.
- **Hybrid Static & Dynamic Dispatch:** Use static wiring by default; selectively enable runtime polymorphism only where you need it.
- **Monolith & Microservice, Unified:** Enjoy the modularity of microservices within an entirely in-process application, and transition to distributed microservices and back without changing your architectural graph.
- **No Hidden Complexity:** No global state, no accidental coupling, no architectural lock-in. Refactoring and onboarding become straightforward—even in large, multi-team codebases.
- **Modern C++, Productively:** ARC narrows the architecture and productivity gap that pushes teams toward other languages for new projects, combining modern practices and clean architecture with C++ performance.

## Compiler Support
- [x] Clang 20+ CMake/Bazel
- [x] GCC 15+ CMake/Bazel (14.2 CMake without `import std;`)
- [x] MSVC 19 (header only)
- [ ] MSVC (modules)

## How to use in your project
Add the following to your CMake, which imports the code for the latest release into your project.
```CMake
include(FetchContent)
FetchContent_Declare(arc URL https://github.com/NasalDaemon/ARC/archive/refs/heads/latest.tar.gz)
FetchContent_MakeAvailable(arc) # makes available arc::headers and arc::module
```
<details>
<summary>Modules</summary>

### Modules
You can link the modularized library (so you can `import arc;`), with
```CMake
target_link_library(your_modules_lib PUBLIC arc::module)
```
To generate module files from the ARC DSL (aka arc), use `target_generate_arc_modules`.
```CMake
target_generate_arc_modules(your_modules_lib
    [MODULE_DIR rel/path=""]
    [GLOB rel/path...]  # explicitly list dirs to search for .ixx.arc files
    [FILES rel/path...] # explicitly list .ixx.arc files
    [EMBED rel/path...] # explicitly list files with embedded arc
)
```
It generates .ixx modules from .ixx.arc files, and .ixx modules from any files listed in EMBED. All generated modules are added to the target.
<details>
<summary>Generating .cpp files for parallel compilation of nodes and faster incremental builds (optional)</summary>

#### Generating .cpp files

To generate {graph}.{node}.cpp files which instantiate your {app.node}:impl implementation partitions for a specified graph, use `target_generate_arc_src`. As each {graph}.{node}.cpp will have visibility of all sibling nodes' module interfaces (via its injected Context), it is important for each {app.node} module interface not to define any non-template functions, leaving as much as possible of the implementation in the respective {app.node}:impl implementation partition.

By having each {app.node}:impl implementation instantiated in a separate {graph}.{node}.cpp file, it allows all listed nodes to be compiled in parallel which can greatly speed up compilation. It also means that changes to a node's implementation only require recompilation of that node's .cpp file.
```CMake
# Consider enabling LTO for production builds so that inter-node function calls are inlined
set_property(TARGET your_modules_lib PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
# Alternatively, enable LTO for your whole project:
# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# Generate .cpp files for listed nodes and add them to the target
target_generate_arc_src(your_modules_lib
    [COMMON_MODULES module.name...]            # modules to import in all generated .cpp files for this graph
    [COMMON_HEADERS path/to/header.hpp...]     # headers to include in all generated .cpp files for this graph
    GRAPH_MODULE your.app.cluster              # module containing the root cluster within which each listed node exists
    GRAPH_TYPE   arc::Graph<your::app::Cluster> # the type of the graph within which each listed node has a context
    NODES                                      # List of pairs: node.path.from.root.cluster   module.name[:impl]
        apple          your.app.apple:impl
        orange         your.app.orange         # :impl is default implementation parition name, so it can be ommitted
        path.to.pear   your.app.pear:node_impl # :node_impl parition is used instead of :impl
)
```
</details>
</details>
<details>
<summary>Headers</summary>

### Headers
You can link the header library (so you can `#include <arc/arc.hpp>`), with
```CMake
target_link_library(your_headers_lib PUBLIC arc::headers)
```
To generate header files from the ARC DSL (aka arc), use `target_generate_arc_headers`.
```CMake
target_generate_arc_headers(your_headers_lib
    [INCLUDE_DIR rel/path=""]
    [GLOB rel/path...]  # explicitly list dirs to search for .hxx.arc files
    [FILES rel/path...] # explicitly list .hxx.arc files
    # explicitly list files with embedded arc
    [EMBED rel/input/path full/include/header.hxx]...
)
```
It generates .hxx headers from .hxx.arc files, and header files from any files listed in EMBED. All files generated from .hxx.arc are added to the target with the same include path as the input .hxx.arc files.
<details>
<summary>Generating .cpp files for parallel compilation of nodes and faster incremental builds (optional)</summary>

#### Generating .cpp files

To generate {graph.node}.cpp files which instantiate your {node}.tpp implementation files for a specified graph, use `target_generate_arc_src`. As each {graph.node}.cpp will have visibility of all sibling nodes' headers (via its injected Context), it is important for each {node}.hpp not define any non-template functions, leaving as much of the implementation in the respective {node}.tpp file as possible (which should not be included in any headers).

By having each {node}.tpp implementation instantiated in a separate {graph.node}.cpp file, it allows all listed nodes to be compiled in parallel which can greatly speed up compilation. It also means that changes to a node's implementation only require recompilation of that node's .cpp file.
```CMake
# Consider enabling LTO for production builds so that inter-node function calls are inlined
set_property(TARGET your_headers_lib PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
# Alternatively, enable LTO for your whole project:
# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# Generate .cpp files for listed nodes and add them to the target
target_generate_arc_src(your_headers_lib
    [UNITY]                                    # instantiate all nodes in a single .cpp file
    [COMMON_HEADERS path/to/header.hpp...]     # other headers to include in all generated .cpp files
    GRAPH_HEADER your/app/cluster.hxx          # header containing the root cluster within which each listed node exists
    GRAPH_TYPE   arc::Graph<your::app::Cluster> # the type of the graph within which each listed node has a context
    NODES                                      # List of pairs: node.path.from.root.cluster path/to/impl.tpp
        apple          your/app/apple.tpp
        orange         your/app/orange.tpp
        path.to.pear   your/app/pear.tpp
        all, in, one   your/app/all.tpp, your/app/in.tpp, your/app/one.tpp
        # nodes all+in+one to be instantiated in the same generated cpp
)
```
</details>
</details>

## Short example ([examples/greet](examples/greet))

```cpp
// File: app/traits.ixx.arc
export module app.traits;

// Traits define interfaces between nodes
trait app::Greeter
{
    greet() const
}

trait app::Responder
{
    respondTo(std::string_view name) const
}
```
```cpp
// File: app/nodes/alice.ixx
export module app.node.alice;

import app.traits;
import arc;
import std;

namespace app::node {

// Alice is a standard node implementing Greeter and Responder
export struct Alice
{
    template<class Context> // Context injected by ARC into the node's state
    struct Node : arc::Node
    {
        // Declares dependency on the Responder trait (provided by another node)
        using Depends = arc::Depends<trait::Responder>;

        // Declares which traits this node implements
        using Traits = arc::Traits<trait::Greeter, trait::Responder>;

        void impl(trait::Responder::respondTo, std::string_view name) const
        {
            std::println("Well met, {}. I am Alice of {} years!", name, age);
        }

        void impl(trait::Greeter::greet) const
        {
            std::println("Hello from Alice! I am {} years old.", age);
            // Resolve dependency (on Bob) using the injected `Context` template parameter
            getNode(trait::responder).respondTo("Alice");
            // The line above can be inlined by the compiler,
            // as getNode and respondTo are both direct calls
        }

        explicit Node(int age) : age(age) {}
        int age; // State specific to this node
    };
};

}
```
```cpp
// File: app/nodes/bob.ixx
export module app.node.bob;

import app.traits;
import arc;
import std;

namespace app::node {

// Bob is a shorthand node with contextless state
// Context is injected into methods by ARC via deducing-this parameter instead
export struct Bob : arc::Node::
    Impl<trait::Greeter, trait::Responder>:: // provides arc::Traits<...> list
    Uses<trait::Responder> // provides arc::Depends<...> list
{
    // impl(trait::Responder::method, ...) redirects here via Impl<..., trait::Responder>
    void respondTo(std::string_view name) const
    {
        std::println("Well met, {}. I am Bob of {} years!", name, age);
    }

    // impl(trait::Greeter::method, ...) redirects here via Impl<trait::Greeter, ...>
    void greet(this auto const& self) // deducing-this `self` parameter has the node context
    {
        std::println("Hello from Bob!");
        // Uses<trait::Responder> provides `getResponder()` aka `getNode(trait::responder)`
        self.getResponder().respondTo("Bob");
        // The line above will be inlined by the compiler
    }

    explicit Bob(int age) : age(age) {}
    int age; // State specific to this node
};

}
```
```cpp
// File: app/clusters/forum.ixx.arc
export module app.cluster.forum;

import app.node.alice;
import app.node.bob;
import app.traits;

// Cluster wires nodes together, satisfying dependencies
cluster app::Forum
{
    alice = node::Alice
    bob = node::Bob

    [trait::Responder]
    alice --> bob  // alice depends on bob for trait::Responder
    alice <-- bob  // bob depends on alice for trait::Responder
    // Can also be expressed simply as:
    // alice <-> bob
}
```
```cpp
// File: app/main.cpp
import arc;
import app.cluster.forum;
import app.traits;

using namespace app;

int main()
{
    // Instantiate the graph: all nodes with dependencies resolved at compile time
    arc::Graph<cluster::Forum> graph{
        .alice{29},
        .bob{30},
    };
    // Graph is a single object on the stack containing all nodes
    static_assert(sizeof(graph) == 2 * sizeof(int));

    // Access nodes through their traits
    graph.alice.asTrait(trait::greeter).greet();
    // Output:
    // Hello from Alice! I am 29 years old.
    // Well met, Alice. I am Bob of 30 years!

    graph.bob.asTrait(trait::greeter).greet();
    // Output:
    // Hello from Bob! I am 30 years old.
    // Well met, Bob. I am Alice of 29 years!

    return 0;
}
```

### Documentation
- [Example project walkthrough using modules](docs/modules-example.md)
- [Defining a node](docs/node-structure.md)
- [ARC cluster: syntax](docs/cluster-syntax.md)
- [ARC domain: scalable clusters](docs/domain-syntax.md)
- [ARC trait: syntax](docs/trait-syntax.md)
- [ARC protocol: lifecycle contracts](docs/protocol-syntax.md)
- [ARC policy: access control syntax for safety-critical systems](docs/policies.md)
- [Embedding ARC DSL into source files](docs/arc-embed.md)
- [Selective runtime polymorphism](docs/runtime-polymorphism.md)
- [Spy trait for global method interception](docs/spy-trait.md)
- [TracerSpy: built-in tracing spy for tests and agentic debugging](docs/tracer-spy.md)

### Compilable [examples](examples/)
- [Filesystem](examples/filesystem): In-memory filesystem with CLI interface
- [Calculator](examples/calculator): CLI calculator with variables, functions, and state persistence
- [Greet](examples/greet): A compilable copy of the [above code](#short-example-examplesgreet)
- [Animal (union)](examples/animals): Demonstrates explicit runtime polymorphism with ARC's `arc::Union` higher-order node (see [runtime polymorphism docs](docs/runtime-polymorphism.md))
- [Animals (virtual)](examples/animals_virtual): Traditional virtual interface version of the Animal example for comparison using `arc::Virtual` higher-order node (see [runtime polymorphism docs](docs/runtime-polymorphism.md))
