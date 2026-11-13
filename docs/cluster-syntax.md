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
    apple = node::Apple
    banana = node::Banana
    cherry = node::Cherry
    sourCherry = node::Cherry
    date = node::Date
    elderberry = node::Elderberry

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
    // Sink nodes (like `elderberry`) may not have any outgoing connections apart from the implicit
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
    using B = trait::Banana, C = trait::Cherry, D = trait::Date

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

### Custom Constructors (optional)

By default, a cluster is an aggregate and can be aggregate-initialised. Declaring one or more constructors makes it non-aggregate and gives it explicit construction. Constructors use C++ parameter and delegation syntax, with designated node initialisers and no body. They must appear directly after the node list, before any connection blocks, and be named after the cluster.

```arc
cluster C
{
    node1 = node::Node1
    node2 = node::Node2

    C(int arg, double d)
        .node1{arg}
        .node2{d, 1}

    // Delegating constructor
    C(int arg) : C(arg, 1.0)

    [Trt] node1 --> node2
}
```

- Omit constructors to retain aggregate initialisation.
- Node initialisers use `.node{args...}` and may only name declared nodes.
- Nodes omitted from the initialiser list are value-initialised.
- A constructor may delegate to another constructor of the same cluster using `: C(args...)`. Delegation cannot be combined with node initialisers.

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

Annotations may be split over several blocks, read as one; a header with trunks reads best with one block each:
```arc
cluster Compute [R = Root]
                [Trunk Repl = Tokeniser + Parser + Evaluator]
                [Trunk Script = Tokeniser + Parser + Functions + Variables]
{
    // ...
}
```
Each annotation appears once across the blocks.

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
    onAnyThread = Node3 : arc::AnyThread : arc::StaticMap<int>
    // Equivalent to:
    // onAnyThread = arc::StaticMap<arc::AnyThread<Node3>, int>
    ordered = Node4 : arc::DynamicMap<int, std::map>
    // Equivalent to:
    // ordered = arc::DynamicMap<Node4, int, std::map>
}
```
The collection wrappers and their map templates are described in [the `arc-node-builtins` skill](../.agents/skills/arc-node-builtins/SKILL.md#collections).

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

Named after **network trunking**, a single link carrying many channels: a trunk bundles several traits into one connection line.

### Cluster Trunks

A cluster declares the trunks it offers in its header, one annotation block each. It declares either one trunk, `[Trunk = A + B]`, or named trunks, `[Trunk Name = ...]`, whose union is then its default trunk. Each trait of a trunk must be exposed to the parent.

```arc
cluster Producer [Trunk Core = A + B]
                 [Trunk Extra = C + D]
{
    // ...

    [A] .. --> a
    [B] .. --> b
    [C] .. --> c
    [D] .. --> d
}

cluster Outer
{
    producer = cluster::Producer
    consumer = cluster::Consumer   // uses A and B
    extra    = node::Extra         // uses C and D
    all      = node::All           // uses A, B, C and D

    [[Producer::Core]]  consumer ==> producer
    [[Producer::Extra]] extra    ==> producer
    [[Producer]]        all      ==> producer
}
```

A client names a trunk as it names the cluster, so `[[Producer::Core]]` where the cluster is visible unqualified, else `[[app::cluster::Producer::Core]]`.

### Extending a Trunk

A named trunk extends others of its cluster by naming them, `Cluster::Name`. This builds a richer face from a smaller one:

```arc
cluster Compute [Trunk Parse = Tokeniser + Parser]
                [Trunk Repl = Compute::Parse + Evaluator]
                [Trunk Script = Compute::Parse + Functions + Variables]
```

Extended trunks merge, each trait once in the order first listed. A trait the trunk lists itself must be new to it. The default trunk is the union, so it is not extended.

### Full Use

A client must use every trait of the trunk it takes, and the compiler checks it:
- a node uses what its `arc::Node::Uses<>` or `arc::Depends<>` lists, optional `T*` entries included;
- a subcluster uses what its nodes use of the links they take from `@parent`, by any kind of link, renamed or not;
- a wrapper uses what it wraps: a collection its element, `arc::Union` its options, and `arc::Lazy`, `arc::Narrow` and `arc::Combine` the nodes within.

```
Node takes a trunk without using all of its traits: declare a trunk on the cluster for just the traits the node uses, ...
  trunk    = app::cluster::Producer::Core
  unused   = app::trait::B
  link     = app.ixx.arc:31:38
  declared = app.ixx.arc:12:9
```

When a client needs part of a trunk, declare a named trunk for that part. For a one-off client, `[[Producer*]]` takes part of a trunk; the part then passes on, so a subcluster taking it with `*` is itself taken with `*`, or through a trunk matching what it uses.

A node that lists no dependencies (a bare `arc::Node`, a mock, an `arc::Virtual`) cannot take a trunk in full. Where such a node takes links from its subcluster's `@parent`, it counts as using all they carry: the check cannot see further. Inline trunks, and trunks `@parent` takes, are not checked for full use.

### One Link per Trait

A trait reaches a node through one link. A trunk cannot be disambiguated inside the node, so a node may not take a trait through a trunk and through another link: a per-trait link, a fan-out, a sink or another trunk. The same holds for `@parent`. Traits match as resolution matches them, so a joined trait overlaps each of its traits. Two per-trait links may overlap, as the node can name either.

The generator rejects the overlaps it can see in the file, and the compiler the rest:

```
Node takes a trait through a trunk and through another link, and a trunk cannot be disambiguated inside the node: take the trait through one link only
  overlap = app::trait::A through [[app::cluster::Producer::Core]] at app.ixx.arc:31:38 and [app::trait::A] at app.ixx.arc:32:24
```

Before C++26 the fields print as `<?>`, and the failed requirement names the trait and the node's two links: `TraitReachesNodeThroughTwoLinks<app::trait::A, ...::client_::TrunkLink0_, ...::client_::TrunkLink1_>`.

### Inline Trunks

A link may bundle traits itself:

```arc
[[A + B + C]]
xyz ==> abc
```

### Bi-directional Trunks

Two clusters taking each other's trunks may share one block, `<=>` separating the two trunks. Its lines use `==>`, `<==` or `<=>`:

```arc
[[Data <=> Compute]]
data <=> compute  // each takes the other's trunk
repl ==> compute  // repl takes Compute's
data <== repl     // repl takes Data's
```

This equals `[[Data]] repl, compute ==> data` and `[[Compute]] repl, data ==> compute`. Each side is a single cluster trunk, which may be named and take `*`.

### Rules
- Trunk blocks use `[[...]]` with `==`-style arrows; `-->` there, or `==>` in `[...]`, is rejected.
- `*` takes one cluster trunk in part; it is rejected on inline trunks and where `@parent` takes the trunk.
- A trunk cannot fan out (`[[X]] a ==> {b, c}`) or be taken twice by one node: fan out its traits instead.
- No sinks, `@notrait` or `<->` in trunk blocks.
- A named trunk is a type in the cluster's scope, so its name must not be a name the cluster uses unqualified (a trait, a type, a template argument or parameter); `Node`, `Trunk`, `SourceLocation` and the cluster's name are taken. A node may not be named `Depends`.

### When to use
- A cluster has faces that several clients use in full: give each face a named trunk, and build larger faces by extension.
- A trait belongs in a trunk only if every client of that trunk uses it; a trunk of one trait is just a per-trait link.
- An inline trunk bundles traits for one link without naming them.
