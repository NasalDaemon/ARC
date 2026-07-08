# Cluster Syntax

Clusters define node dependencies and state. They are defined in `.ixx.arc` (modules) or `.hxx.arc` (headers) files. ARC's DSL provides a visual way to describe these graphs.

Use `target_generate_arc_modules()` or `target_generate_arc_headers()` in CMake to process these files.

## Quick Reference

There are special node and trait markers for common dependency types. Either the keyword (e.g. `@parent`) or its shorthand (e.g. `..`) can be used in the DSL.

| Keyword | Shorthand | Description |
|--------|-------------|-------------|
| `@parent`| `..` | Parent cluster |
| `@global`| `^` | Global node |
| `@all`| `*` | All sibling nodes and the parent (sink connection blocks only) |
| `@notrait`| `~` | No-trait connection marker |

## Defining a Cluster

Clusters must be namespaced, either via a wrapping `namespace` block or a using a fully qualified name. The generated cluster type is placed inside `namespace cluster` within the enclosing namespace — so `cluster FruitSalad` inside `namespace my::app` produces `my::app::cluster::FruitSalad`.

```arc
namespace my::app {

cluster FruitSalad [Context, Root] // Optional type annotations
{
    // 1. Node definitions
    apple = Apple
    banana = Banana
    cherry = Cherry
    sourCherry = Cherry
    date = Date
    elderberry = Elderberry

    // 2. (Optional) enable all connections to @global node (must come first)
    [[@global]] @all ==> @global
    // Allows any node in this cluster to connect to the global node
    // by any trait it implements via getGlobal(trait).
    // Alternatively, you can specify a subset of nodes:
    // [[@global]] apple, banana ==> @global
    // NOTE: `[[Trait]]` and `==>` shows that multiple traits are being served by the same connection

    // 3. Sink connection block (must come before normal connection blocks)
    [trait::Elderberry] @all --> elderberry
    // Shorthand (omitting the arrow):
    // [trait::Elderberry] elderberry
    // Connects all other sibling nodes and the parent to elderberry
    // Equivalent to:
    // [trait::Elderberry]
    // elderberry <-- @parent, apple, banana, cherry, sourCherry, date
    // Sink nodes (like `elderBerry`) may not have any outgoing connections apart from the implicit
    // connections to global nodes and other sink nodes
    // Sink traits cannot be used in any further connection blocks

    // 4. Connection block (using both arrow directions)
    [trait::Apple]
    banana --> apple
    apple <-- cherry
    // equivalent to:
    // [trait::Apple]
    // banana --> apple
    // cherry --> apple
    // where banana.getNode(trait::apple) ~= apple.asTrait(trait::apple)
    // and cherry.getNode(trait::apple) ~= apple.asTrait(trait::apple)

    // 5. Aliases for traits
    using B = trait::Banana, C = trait::cherry, D = trait::Date

    // 6. Many-to-one (and using trait alias)
    [B] apple, cherry --> banana
    // equivalent to:
    // [trait::Banana]
    // apple --> banana
    // cherry --> banana

    // 7. Bi-directional connections
    [C <-> D]
    cherry <-> date
    cherry <-- apple, banana
    apple, banana --> date
    // equivalent to:
    // [trait::Cherry]
    // cherry <-- date
    // [trait::Date]
    // cherry --> date
    // [trait::Cherry]
    // cherry <-- apple, banana
    // [trait::Date]
    // apple, banana --> date

    // 8. Trait disambiguation (using trait renaming)
    [trait::Cherry]
    apple, banana (trait::SourCherry) --> sourCherry
    // equivalent to:
    // apple  (trait::SourCherry) --> (trait::Cherry) sourCherry
    // banana (trait::SourCherry) --> (trait::Cherry) sourCherry
    // where apple.getNode(trait::sourCherry) ~= sourCherry.asTrait(trait::cherry)
    // and banana.getNode(trait::sourCherry) ~= sourCherry.asTrait(trait::cherry)
    // This disambiguates cherry and sourCherry from the point of view of apple and banana,
    // although cherry and sourCherry are both just cherries from their own points of view

    // 9. Daisy-chain
    [trait::FruitSalad]
    @parent --> apple --> banana --> cherry --> sourCherry --> date
    // equivalent to:
    // [trait::FruitSalad]
    // @parent --> apple
    //             apple --> banana
    //                       banana --> cherry
    //                                  cherry --> sourCherry
    //                                             sourCherry --> date

    // Note: when FruitSalad is used as a node in a parent cluster,
    // trait::FruitSalad transparently connects to apple

    // 10. Explicit fan-out connections (one-to-many)
    [trait::ChopFruit]
    @parent --> {apple, banana, cherry, date, sourCherry}
    // which automatically generates a special intermediate node
    // `_parentRepeater0 = arc::Repeater<trait::ChopFruit, 5>` with the connections:
    // [trait::ChopFruit]
    // .. --> _parentRepeater0 (arc::RepeaterTrait<0>) --> apple
    //        _parentRepeater0 (arc::RepeaterTrait<1>) --> banana
    //        _parentRepeater0 (arc::RepeaterTrait<2>) --> cherry
    //        _parentRepeater0 (arc::RepeaterTrait<3>) --> date
    //        _parentRepeater0 (arc::RepeaterTrait<4>) --> sourCherry
    // where any trait::ChopFruit::method call is repeated by the repeater node
    // to apple, then banana, then cherry, then date, then sourCherry
    // as if calling for Is 0..4:
    // _parentRepeater0.getNode(arc::RepeaterTrait<Is>{}).method(args...)

    // Note: when FruitSalad is used as a node in a parent cluster,
    // trait::ChopFruit transparently connects to _parentRepeater0

    // 10.2. Implicit fan-out connections (one-to-many over multiple lines)
    [trait::CrushFruit]
    @parent --> apple
    @parent --> banana
    @parent --> cherry
    @parent --> date
    @parent --> sourCherry
    // This has equivalent semantics to the above fan-out example (but with a different trait)
    // Notes:
    // 1. This cannot be combined with the inline explicit fan-out syntax {node1, node2}
    // 2. All targets of a repeated trait connection from the same node must
    //    be listed in a single connection block

    // 11. Explicitly redirecting trait to the global node
    [trait::Log]
    apple --> @global
    // which resolves trait::Log to the respective global node
    // Equivalent to:
    // [trait::Log]
    // apple --> (arc::Global<trait::Log>) @global
    // This is only necessary if `apple` expects `trait::Log` to be resolved
    // using `getNode` instead of `getGlobal`
}

}
```

### Namespace and Type Annotations

**Inline Namespaces:**
```
cluster my::app::Cluster { ... }
```
This produces `my::app::cluster::Cluster`.

**Type Annotations:**
Access `Context`, `Root`, or `Info` types by listing them in brackets. You can also alias them:
```arc
cluster MyCluster [C = Context, R = Root]
{
    node = R::NodeType
}
```

### Templated node definitions
There are many higher-order nodes provided by ARC that are templated on other nodes, such as `arc::InGroup`, `arc::OnThread`, and `arc::StaticMap`. The DSL provides a convenient syntax for using these without needing to write out the full template syntax. The general form is `nodeName = UnderlyingNode : Wrapper1<Args> : Wrapper2<Args> ...` where each wrapper is a higher-order node taking an underlying node as its first template argument. This allows you to quickly and easily read the node definitions from left to right, starting with the underlying node and then seeing how it is wrapped by various higher-order nodes. For example:
```arc
cluster MyCluster
{
    topSecret = Node1 : arc::InGroup<policy::Classification::TopSecret>
    // Equivalent to:
    // topSecret = arc::InGroup<Node1, policy::Classification::TopSecret>
    onThread0 = Node2 : arc::OnThread<0>
    // Equivalent to:
    // onThread0 = arc::OnThread<Node2, 0>
    onAnyThread = Node3 : arc::OnAnyThread : arc::StaticMap<int>
    // Equivalent to:
    // onAnyThread = arc::StaticMap<arc::OnAnyThread<Node3>, int>
}
```

## No-Trait Connections [`@notrait`]

Use `@notrait` (or `~`) for traitless, type-safe connections. The client gains access to all public members of the provider. Use `@notrait` for internal wiring where no interface abstraction is needed, or when there will only ever be one provider of the functionality.

### Usage
- **Internal wiring** where no interface is needed.
- **Prototyping** before defining formal traits.
- **Single implementation** scenarios where abstraction is unnecessary.

```cpp
cluster MyCluster
{
    client = Client
    provider = Provider
    [@notrait] client --> provider
    // Equivalent to:
    // [arc::NoTrait<Provider>]
    // client --> provider
}

// Provider implementation
struct Provider : arc::Node {
    using Traits = arc::NoTraits;
    void doWork();
};

// Client consumption
auto p = getNode(arc::noTrait<Provider>);
p->doWork();
```
### Multiple sinks in one declaration:
Multiple no-trait sinks can be declared in one line. This is only possible for no-trait connections.
```
[@notrait] logger, metrics
```
## Caveats
- **No Mixing:** Cannot mix no-trait and named trait connections for the same target node.
- **Special Nodes:** `@notrait` (`~`) is not supported for `@global` (`^`) or `@parent` (`..`) nodes; use `arc::NoTrait<NodeHandle>` or a named trait.
- **Migration:** To formalize an interface, replace `[@notrait]` with `[trait::Name]` and update `getNode` calls.

## Trunking Connections `[[...]]` + `==>`

Named after **network trunking** — a single high-capacity link carrying many separate channels between two endpoints. Trunks bundle multiple traits into one connection line.

### Cluster Trunks

A cluster declares a curated **trunk** of exposed traits in its header, and a sibling node or subcluster carries all of those traits over one connection line.

```arc
cluster Producer [Trunk = A + B + C]   // subset; D exposed but not in trunk
{
    // ...

    [A] .. --> a
    [B] .. --> b
    [C] .. --> c
    [D] .. --> d
}

cluster Consumer
{
    // ...

    [A] x --> ..
    [B] y --> ..
    [C] z --> ..
}

cluster Outer
{
    producer = cluster::Producer
    consumer = cluster::Consumer
    node = node::Node

    [[cluster::Producer]]
    consumer ==> producer    // consumer gets A + B + C
    [D]
    node --> producer        // D available as normal
}
```

### Inline Trunking

You can also define a trunk inline without a cluster declaration.

```arc
cluster MyCluster
{
    abc = node::ABC
    xyz = node::XYZ

    [[A + B + C]]
    xyz ==> abc
}
```

Inline trunks are useful when:
- You want to bundle traits without defining a cluster or a new joined trait
- You need a one-off trunk for a specific connection

### Bi-directional Trunks

Two clusters that depend on each other's trunks can be wired in a single block using a `<=>` separator in the header. The block's left and right trunks behave like the two sides of a `[A <-> B]` bi-trait header, and the connection lines may use `==>`, `<==`, or `<=>` per pair.

```arc
cluster Outer
{
    data    = cluster::Data
    compute = cluster::Compute
    repl    = node::Repl

    [[cluster::Data <=> cluster::Compute]]
    data <=> compute  // data and compute consume each other's trunks
    repl ==> compute  // repl consumes Compute trunk
    data <== repl     // repl consumes Data trunk (reversed form)
}
```

This is equivalent to:

```arc
cluster Outer
{
    // ...
    [[cluster::Data]]    repl, compute ==> data
    [[cluster::Compute]] repl, data    ==> compute
}
```

The bi-trunk form is a matter of taste — it makes the symmetric relationship explicit but mixes arrow directions in one block.

Both sides of a bi-trunk connetion block must be single cluster trunks (no `+` on either side).

### Rules
- `[Trunk = T1 + T2 + ...]` declares the cluster's curated public surface. Every listed trait must be exposed via a matching `[T] .. --> node` parent connection — the generator checks this.
- Trunk consumption uses **double brackets** `[[...]]` and **`==`-style arrows** (`==>`, `<==`, `<=>`). Single-bracket `[...]` with `==>` is rejected, and `[[...]]` with `-->` is rejected.
- A `[[...]]` block can contain a single cluster (uses cluster's trunk), multiple traits joined with `+` (inline trunk), or two cluster trunks joined with `<=>` (bi-trunk).
- Bi-directional connections (`<=>`) are only allowed inside a bi-trunk block `[[A <=> B]]`; in a uni-trunk block they're rejected because both directions would carry the same trunk.
- The bi-trunk separator must be `=`-style (e.g. `<=>`); `<->` is rejected to match the `==`-style arrow convention.
- Inline trunks (`+`) cannot be bi-directional — each side of `<=>` must be a single cluster trunk.
- No bi-trait `[[X <-> Y]]` (use `<=>` instead), no sinks, no `@notrait`.

### When to use
- A subcluster has a coherent public face (multiple related traits) and a sibling consumer needs most of it. The named trunk replaces a hand-maintained `trait Composite = A + B + C` alias.
- You want to bundle traits inline without defining a cluster or naming a new joined trait
