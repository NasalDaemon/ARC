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

Clusters must be namespaced, either via a wrapping `namespace` block or a qualified name.

```cpp
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

    // 2. Sink connection block (must come before normal connection blocks)
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

    // 3. Connection block (using both arrow directions)
    [trait::Apple]
    banana --> apple
    apple <-- cherry
    // equivalent to:
    // [trait::Apple]
    // banana --> apple
    // cherry --> apple
    // where banana.getNode(trait::apple) ~= apple.asTrait(trait::apple)
    // and cherry.getNode(trait::apple) ~= apple.asTrait(trait::apple)

    // 4. Aliases for traits
    using B = trait::Banana, C = trait::cherry, D = trait::Date

    // 5. Many-to-one (and using trait alias)
    [B] apple, cherry --> banana
    // equivalent to:
    // [trait::Banana]
    // apple --> banana
    // cherry --> banana

    // 6. Bi-directional connections
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

    // 7. Trait disambiguation (using trait renaming)
    [trait::Cherry]
    apple, banana (trait::SourCherry) --> sourCherry
    // equivalent to:
    // apple  (trait::SourCherry) --> (trait::Cherry) sourCherry
    // banana (trait::SourCherry) --> (trait::Cherry) sourCherry
    // where apple.getNode(trait::sourCherry) ~= sourCherry.asTrait(trait::cherry)
    // and banana.getNode(trait::sourCherry) ~= sourCherry.asTrait(trait::cherry)
    // This disambiguates cherry and sourCherry from the point of view of apple and banana,
    // although cherry and sourCherry are both just cherries from their own points of view

    // 8. Daisy-chain
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

    // 9. Explicit fan-out connections (one-to-many)
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

    // 9.2. Implicit fan-out connections (one-to-many over multiple lines)
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

    // Explicitly redirecting trait to the global node
    [trait::Log]
    apple --> @global
    // which resolves trait::Log to the respective global node
    // Equivalent to:
    // [trait::Log]
    // apple --> (arc::Global<trait::Log>) @parent
    // This is only necessary if `apple` expects `trait::Log` to be resolved
    // using `getNode` instead of `getGlobal`
}

}
```

### Namespace and Type Annotations

**Inline Namespaces:**
```cpp
cluster my::app::Cluster { ... }
```

**Type Annotations:**
Access `Context`, `Root`, or `Info` types by listing them in brackets. You can also alias them:
```cpp
cluster MyCluster [C = Context, R = Root] {
    node = R::NodeType
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
    using Traits = arc::NoTraits<Provider>;
    void doWork();
};

// Client consumption
auto p = getNode(arc::noTrait<Provider>);
p->doWork();
```
### Multiple sinks in one declaration:
```cpp
[@notrait] logger, metrics
// Multiple no-trait sinks can be declared in one line (only possible for no-trait connections)
```
## Caveats
- **No Mixing:** Do not mix no-trait and named trait connections for the same target node.
- **Special Nodes:** `@notrait` (`~`) is not supported for `@global` (`^`) or `@parent` (`..`); use `arc::NoTrait<NodeHandle>` or a named trait.
- **Migration:** To formalize an interface, replace `[@notrait]` with `[trait::Name]` and update `getNode` calls.
