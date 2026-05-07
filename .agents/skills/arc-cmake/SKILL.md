---
name: arc-cmake
description: ARC CMake integration, build commands, adding nodes and tests
user-invocable: true
---

# ARC CMake Integration

For DSL syntax see `/arc-dsl`. For node authoring patterns see `/arc-nodes`.

## Core Commands

**`target_generate_arc_modules`** — generates `.ixx` from `.ixx.arc` files:
```cmake
target_generate_arc_modules(my_target GLOB src)             # Auto-discover in directory
target_generate_arc_modules(my_target FILES src/t.ixx.arc)  # Explicit files
target_generate_arc_modules(my_target EMBED test.cpp)       # Embedded arc DSL in source

# Can combine GLOB/FILES/EMBED as needed
target_generate_arc_modules(my_target
    GLOB
        src
    FILES
        src/t.ixx.arc
        src/u.ixx.arc
    EMBED
        test.cpp
)
```

**`target_generate_arc_headers`** — generates `.hxx` from `.hxx.arc` files (for header-based projects):
```cmake
target_generate_arc_headers(my_target GLOB src)
target_generate_arc_headers(my_target FILES src/t.hxx.arc)

# Can combine GLOB/FILES/EMBED as needed
target_generate_arc_headers(my_target
    GLOB
        src
    FILES
        src/t.hxx.arc
        src/u.hxx.arc
    EMBED
        test.cpp src/path/to/generated/header.hxx  # The path to #include
)
```

**`target_generate_arc_src`** — explicit graph instantiation for parallel compilation. **Required** when contextful nodes use `.impl.ixx`/`.tpp` split implementations. **Omit** all contextless nodes without split implementations `.impl.ixx`/`.tpp`. In other words, if it already has a `.cpp` implementation, there is no reason to list it in `target_generate_arc_src`.

For the clusters:
```arc
export module app.clusters;

import app.nodes.read_file;
import app.nodes.write_file;

cluster app::App [Root]
{
    counter = Root::Counter // Node type decided by the Root
    fs = cluster::Filesystem

    // ... some connections
}

cluster app::Filesystem
{
    readFile = node::ReadFile
    writeFile = node::WriteFile // Write file has no dependencies, so contextless with .cpp implementation

    // ... some connections
}
```
```cmake
# Each GRAPH_TYPE needs its own independent target_generate_arc_src:
target_generate_arc_src(my_target
    GRAPH_MODULE   app.clusters # Module containing the graph type
    GRAPH_TYPE     "arc::Graph<app::cluster::App, app::CustomRoot>"  # Full graph type
    # COMMON_MODULES (optional) are imported in all generated sources,
    # for shared dependencies like a custom root or nodes in the root
    COMMON_MODULES
        app.roots         # For app::CustomRoot
        app.node.counter  # For Root::Counter (app::CustomRoot::Counter = app::node::Counter)
    NODES
        counter             app.node.counter       # nodeNameInCluster module.name
        fs.readFile         app.node.read_file     # Nested cluster node paths use dots
        # Note: fs.writeFile is contextless with .cpp implementation, so we do NOT include it here
)

# For each node with a split implementation under test:
target_generate_arc_src(my_target
    GRAPH_MODULE app.node.node1                        # Module containing the node under test
    GRAPH_TYPE   "arc::test::Graph<app::node::Node1>"  # Full test graph type as used in the test cases
    NODES
        node app.node.node1
)
```

For header-based projects, use `GRAPH_HEADER` instead of `GRAPH_MODULE`:
```cmake
target_generate_arc_src(my_target
    GRAPH_HEADER math.hxx
    GRAPH_TYPE "arc::Graph<cluster::MathCluster>"
    COMMON_HEADERS common.hxx  # Optionally include any common headers needed for the graph
    NODES
        piCache path/to/impl/pi_cache.tpp
)
```

## Adding a New Node

1. Create `src/nodes/<name>.ixx` with module and struct
2. If context-injected: create `src/nodes/impl/<name>.impl.ixx`
3. If contextless with large methods: create `src/nodes/impl/<name>.cpp`
4. Add the `.ixx` and `.impl.ixx` files to `target_sources(app_modules ... FILE_SET CXX_MODULES FILES ...)`
5. If `.cpp`: add to the `add_library(app_modules OBJECT ...)` source list
6. Add node to the appropriate cluster in `clusters.ixx.arc`
7. If using `.impl.ixx`/`.tpp`: add a NODES entry to the relevant `target_generate_arc_src` using the name of the node as it appears in the cluster definition

## Adding Tests for a Node

1. Create `tests/test_<name>.cpp`
2. Add the test file to `add_executable(app_tests ...)`
3. Add a `target_generate_arc_src` call for the test graph type
4. If using embedded test clusters: add `target_generate_arc_modules(app_tests EMBED tests/test_<name>.cpp)`
