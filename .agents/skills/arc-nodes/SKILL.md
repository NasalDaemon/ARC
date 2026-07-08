---
name: arc-nodes
description: >
    ARC node patterns, contextless vs context-injected, .impl.ixx convention.
    Load when: reading or writing node implementations, planning cluster structure, or exploring built-in node types.
user-invocable: true
---

# Node Patterns & Built-in Types

## Node Namespace Convention

By convention, define all nodes inside `namespace node` (e.g. `app::node::Counter`). This mirrors how DSL-generated entities are placed in their respective namespaces (`trait`, `cluster`, `domain`, `policy`). If not using `namespace node`, suffix node type names with `Node` (e.g. `CounterNode`) for clarity.

## Contextless Node (no dependencies)

Use `arc::NodeImpl<Traits...>`. Methods are regular member functions. Move large method bodies to a separate `.cpp` file for parallel compilation.

```cpp
// nodes/my_node.ixx
export module app.node.my_node;
import app.traits;
import arc;

namespace app::node {

export struct MyNode : arc::NodeImpl<trait::MyTrait>
{
    void myMethod() const;  // Declared here, defined in .cpp
    int inlineMethod() const { return 42; }  // Small methods stay inline
};

}

// nodes/impl/my_node.cpp
module app.node.my_node;
import std;

namespace app::node {

void MyNode::myMethod() const { std::println("hello"); }

}
```

## Contextless Node (with dependencies)

Use the fluent `arc::Node::Uses<...>::Impl<...>` API (or `arc::NodeUses<...>` if there are no traits to implement). Methods accessing dependencies use deducing-this (`this auto& self`) to get typed access via named getters.

```cpp
export struct MyNode : arc::Node::
    Impl<trait::MyTrait>::
    Uses<trait::Logger>
{
    // self.getLogger() returns the Logger dependency
    void doWork(this auto& self) { self.getLogger().log("working"); }
};
```

The fluent builder generates:
- `using Depends = arc::Depends<trait::Logger>;`
- `using Traits = arc::Traits<trait::MyTrait>;`
- Named getter `getLogger()` for each `Uses` dependency
- Named converter `asMyTrait()` for each `Impl` trait
- Direct method forwarding: `self.doWork()` calls `self.impl(trait::MyTrait::doWork{})`

`Uses` and `Impl` can be used independently or chained in any order. Shortcuts: `arc::NodeImpl<Trait...>` (impl-only), `arc::NodeUses<Dep...>` (deps-only).

## Dependency Declaration

All nodes should declare their dependencies with `arc::Depends` (or `arc::Uses` shortcut).

```cpp
using Depends = arc::Depends<
    trait::Logger,           // Required dependency
    trait::Metrics*,         // Optional dependency (only checked when used)
    arc::Global<trait::Spy>  // Global node dependency (via getGlobal)
>;
```

In clusters this is optional, but in **domains** it is mandatory.

## Context-Injected Node (complex dependencies, type resolution)

Use the nested `template<class Context> struct Node` pattern. Required when resolving types from dependencies or the graph root. Method implementations go in `.impl.ixx` for parallel compilation.

```cpp
// nodes/my_node.ixx
export module app.node.my_node;
import app.traits;
import arc;

namespace app::node {

export struct MyNode
{
    template<class Context>
    struct Node : arc::Node::
        Impl<trait::Filesystem>::
        Uses<trait::Storage>
    {
        // Type resolution from dependencies
        struct Types
        {
            using StorageTypes = arc::ResolveTypes<Node, trait::Storage>;
            using Result = StorageTypes::Result;
        };

        // Type resolution from graph root
        using Config = Context::Root::Config;

        auto read(std::string_view path) const -> Types::Result;

        // State
        int cache_size_ = 0;
    };
};

}
```

## .impl.ixx Pattern (implementation partition)

Separates template method bodies from declarations. Required for context-injected nodes to enable parallel compilation.

```cpp
// nodes/impl/my_node.impl.ixx
module app.node.my_node:impl;

import app.node.my_node;
import app.traits;
import std;

// Macro avoids repeating the template prefix on every method
#define MY_NODE \
    template<class Context> \
    auto MyNode::Node<Context>

namespace app::node {

MY_NODE::read(std::string_view path) const -> Types::Result
{
    return getStorage().get(path);
}

}
```

**Important:** When using `.impl.ixx`, you MUST add a `target_generate_arc_src` call in CMakeLists.txt to explicitly instantiate the graph, otherwise the linker won't find the template definitions.

## Detached Interface

For nodes that want to separate trait implementations into a nested struct (e.g. when a node implements multiple traits with conflicting method names):

```cpp
struct FruitBasket : arc::Node
{
    struct AppleImpl; // detached
    struct PearImpl; // extends FruitBasket

    using Traits = arc::Traits<
        trait::Orange,              // methods: FruitBasket, types: FruitBasket::Types
        trait::Apple(AppleImpl),    // methods: AppleImpl, types: FruitBasket::Types
        trait::Pear(PearImpl)       // methods: PearImpl, types: FruitBasket::Types
    >;

    int oranges = 5;
    int apples = 3;
    int pears = 3;

    int impl(trait::Orange::take) { return oranges--; }

    struct AppleImpl : arc::DetachedInterface
    {
        // Accesses parent state via explicit object parameter
        int impl(this auto& self, trait::Apple::take) { return self->apples--; }
    };
};

struct PearImpl : FruitBasket
{
    // Accesses parent state directly since it's a subclass
    int impl(trait::Pear::take) { return pears--; }
};
```

---

## Built-in Node Types

For higher-order wrappers (`arc::Union`, `arc::Virtual`, collections (`arc::StaticMap`/`arc::DynamicMap`/`arc::StaticIndex`/`arc::DynamicIndex`), `arc::DataStore`, threading, access control, `arc::Spy`, and keys), load the `/arc-node-builtins` skill.
