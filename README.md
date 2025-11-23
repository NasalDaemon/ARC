# ARC: The New Standard for C++ Architecture

**ARC** is the C++23 framework that turns architectural intent into enforceable reality—giving modern projects everything they need: uncompromising runtime speed, explicit and maintainable architecture, lightning-fast modular builds, and seamless scalability from modular monolith to microservices. This is a standalone library which may be imported either as a C++20 module or a header-only library.

## Why ARC?

- **Architecture as First-Class Code:** Define your system as composable nodes with explicit dependencies and interfaces. Your boundaries are type-safe and compiler-enforced—not documentation that drifts out of sync.
- **True Zero Overhead:** Not only at graph construction, but also at trait resolution and method invocation. There are no vtables, heap-allocations, runtime lookups, or hidden layers—just direct, inlined calls as if you hand-wrote them.
- **Blazing Build Speed:** Each node can compile independently (even when building with C++ modules), so you never pay for modularity with slow builds.
- **Testability by Default:** Swap in mocks or stubs anywhere, with full compile-time safety and zero runtime cost.
- **No Intrusive Macros or Boilerplate:** Enjoy clean, modern C++.
- **Hybrid Static & Dynamic Dispatch:** Use static wiring everywhere for maximum performance; selectively enable runtime polymorphism only where you need it—effortlessly.
- **Monolith & Microservice, Unified:** Instantly switch any node between in-process, out-of-process, or back—at any stage. Test, scale, and adapt without friction or rewrites.
- **Thread Safety by Design:** Thread affinity is enforced at compile time, eliminating entire classes of concurrency bugs before they start.

## What Makes ARC Unique?

- **Explicit Structure:** Build your application from cohesive nodes that implement well-defined traits (interfaces) and declare their dependencies explicitly. ARC wires everything together at compile time into an efficient, type-safe graph.
- **No More Trade-Offs:** Traditional approaches force you to choose between performance, modularity, testability, or maintainability. ARC delivers all, without compromise.
- **No Hidden Complexity:** No global state, no accidental coupling, no architectural lock-in. Refactoring and onboarding become straightforward—even in large, multi-team codebases.
- **Deployment Agnostic:** Effortlessly migrate between monolith and microservices as your needs evolve, making it perfect for both greenfield and long-lived projects.
- **Modern C++, Productively:** ARC erases the architectural and productivity gap that often pushes teams toward other languages for new projects. Now, you can have modern practices, clean architecture, and C++ performance in one package.

## How It Works

ARC structures applications using three core concepts:

1. **Traits** - Interfaces that define contracts between nodes (e.g., `Greeter`, `Responder`)
2. **Nodes** - Self-contained units that implement traits and declare which traits they depend on
3. **Clusters** - Compositions that wire nodes together, satisfying all dependencies at compile time

The result is a **Graph**—a single, efficient object containing all your nodes with their dependencies resolved. The compiler can inline across node boundaries, giving you modularity without overhead.

## Compiler Support
- [x] Clang 20+
- [x] GCC 15+ (14.2 without `import std;`)
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
trait app::trait::Greeter
{
    greet()
}

trait app::trait::Responder
{
    respondTo(std::string_view name)
}
```
```cpp
// File: app/alice.ixx
export module app.alice;

import app.traits;
import arc;
import std;

namespace app {

// Alice is a short-hand node with contextless state
// It implements Greeter and Responder
export struct Alice : arc::Node
{
    // Declares dependency on the Responder trait (provided by another node)
    using Depends = arc::Depends<trait::Responder>;

    // Declares which traits this node implements
    using Traits = arc::Traits<Alice, trait::Greeter, trait::Responder>;

    void impl(this auto const& self, trait::Greeter::greet)
    {
        std::println("Hello from Alice! I am {} years old.", self.age);
        // Resolve dependency via explicit object parameter `self`, containing the node's context
        self.getNode(trait::responder).respondTo("Alice");
        // The line above can be inlined by the compiler, as getNode and respondTo are both direct calls
    }

    void impl(trait::Responder::respondTo, std::string_view name) const
    {
        std::println("Well met, {}. I am Alice of {} years!", name, age);
    }

    explicit Alice(int age) : age(age) {}
    int age; // State specific to this node
};

}
```
```cpp
// File: app/bob.ixx
export module app.bob;

import app.traits;
import arc;
import std;

namespace app {

// Bob is full node with contextful state
export struct Bob
{
    template<class Context>
    struct Node : arc::Node
    {
        using Depends = arc::Depends<trait::Responder>;
        using Traits = arc::Traits<Node, trait::Greeter, trait::Responder>;

        void impl(trait::Greeter::greet) const
        {
            std::println("Hello from Bob!");
            // Can call getNode directly, as Context is already injected into the state
            getNode(trait::responder).respondTo("Bob");
            // The line above can be inlined by the compiler
        }

        void impl(trait::Responder::respondTo, std::string_view name) const
        {
            std::println("Well met, {}. I am Bob of {} years!", name, age);
        }

        explicit Node(int age) : age(age) {}
        int age; // State specific to this node
    };
}

}
```
```cpp
// File: app/forum.ixx.arc
export module app.forum;

import app.alice;
import app.bob;
import app.traits;

// Cluster wires nodes together, satisfying dependencies
cluster app::Forum
{
    alice = Alice
    bob = Bob

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
import app.forum;
import app.traits;

using namespace app;

int main()
{
    // Instantiate the graph: all nodes with dependencies resolved at compile time
    arc::Graph<Forum> graph{
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

### Documentation and Examples
- [Example project using modules](docs/modules-example.md)
- [Defining a node](docs/node-structure.md)
- [ARC cluster: syntax](docs/cluster-syntax.md)
- [ARC domain: scalable clusters](docs/domain-syntax.md)
- [ARC trait: syntax](docs/trait-syntax.md)
- [Embedding ARC DSL into source files](docs/arc-embed.md)
- [Selective runtime polymorphism](docs/runtime-polymorphism.md)
