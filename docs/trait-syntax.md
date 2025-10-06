# Trait syntax

A trait is an interface presented by one node to another to satisfy a dependency. In a [cluster](cluster-syntax.md), all node dependencies are satisfied by connecting to the appropriate providing nodes by the name of the trait they implement. As a result, on each dependent node, a `my::TraitName::method` on the providing node can be invoked via `getNode(my::traitName).method(args...)`, and a `TypeName` can be resolved via `arc::ResolveTypes<Node, my::TraitName>::TypeName`. See the example [here](modules-example.md#sessionsixx).

With ARC, you can use a special DSL (Domain Specific Language) called "arc" to define traits (and [clusters](cluster-syntax.md)). The DSL is designed to specify and constrain the methods that can be called from the result of `getNode(my::traitName)`, and also to list any further type requirements of each implementation. Traits are defined in separate files next to your other source files with the extension `.ixx.arc` (module) or `.hxx.arc` (header).

The provided CMake functions `target_generate_arc_modules()` and `target_generate_arc_headers()` automatically generate `.ixx` or `.hxx` files from the `.ixx.arc` and `.hxx.arc` files found recursively in the source directory, and add them to the target. For generated `.hxx` files, each resulting include path matches the include path of the respective `.hxx.arc` file.

## Trait `.arc` file template

### Header includes and module imports

Header includes and module imports should be listed at the top of the file. [See here](arc-files.md) for further information.

### Defining a trait

**Note:** When defining a trait, it must either be wrapped in a namespace (which cannot be anonymous) or given a qualified name including the namespace.

```cpp
namespace my {

// Non-template methods are added to the `arc::Implements<Impl, my::Trait>` concept,
// which is enforced in the call to `getNode` and `asTrait`.
trait Trait
{
    // `getNode(trait).method(int)` can be called in a const context
    // the return type is unconstrained, i.e. `decltype(auto)`
    // any implementation missing this overload will fail in `getNode`
    method(int i) const

    // `getNode(trait).method(double)` can be called in a mutable context
    // the return type must be convertible to `double`, which is what will be returned
    // any implementation missing this overload will fail in `getNode`
    method(double d) -> double
}

// Template methods are supported, but are not added to the `arc::Implements<Impl, my::Complex>` concept,
// so their existance is *not* enforced in the call to `getNode` or `asTrait`.
// Instead, compilation only fails when the method is called and it is not implemented or conforming.
trait Complex
{
    // `getNode(complex).unconstrained(...)` can be called with any set of arguments
    // the return type is unconstrained, i.e. decltype(auto)
    // any non-conformant implementation will compile until the method is called
    unconstrained(...)

    // `getNode(complex).complex<0>(0, 0, 0, 0, 0.0)` can be called
    // the return type must be exactly `double`
    // any non-conformant implementation will compile until the method is called
    template<std::size_t I, class T, class U, std::same_as<int> Int>
    complex(T t, U&& u, Int i, auto a, std::same_as<double> auto d) -> std::same_as<double>
}

// If requirements on the trait types need to be enforced, use annotations.
// NB: Without annotations, the type names are implementation-defined,
// and so should not be used inside the trait.
trait Trait2 [T = Types]
{
    type Int
    // does not need the annotation - equivalent to:
    // requires typename T::Int

    // Can add extra constraint to Int (using name specified in annotation)
    requires std::integral<typename T::Int>
}

// If requirements on the root types need to be enforced
trait Trait3 [R = Root]
{
    root Int
    // does not need the annotation - equivalent to:
    // requires typename R::Int

    // Can add extra constraint to Int (using name specified in annotation)
    requires std::integral<typename R::Int>
}

// It is not necessary to give Types or Root alternative names in the annotation
trait Trait4 [Types, Root]
{
    requires std::integral<typename Types::Int>
    requires std::integral<typename Root::Int>

    // Can add arbitrary concept constraints, including:
    // `impl_` for mutable implementation, `const_` for const implementation

    // Added to arc::Implements<Impl, Trait4>:
    // mutable implementation have have mutableMethod(std::string)
    requires (std::string s) {
        impl_.mutableMethod(s)
    }

    // Added to arc::Implements<Impl, Trait4>
    // const implementation have have constMethod(int)
    requires const_.constMethod(0)
}

// `Trait` interface is provided and enforced, but must be resolved using name `AltTrait`.
// Useful for disambiguation between two dependencies implementing the same trait.
trait AltTrait = Trait

// `Trait` and `Complex` interfaces are both provided and enforced.
// A node implementing `TraitAndComplex` also implicitly implements `Trait` and `Complex`
// and can be resolved as such, as long as the `Trait`/`Complex` connection exists between nodes.
trait TraitAndComplex = Trait + Complex

}
```

### Qualifying trait namespace inline

You can avoid wrapping the trait in a namespace, like:
```cpp
trait my::Trait
{
    // ...
}
```

**Note:** Namespaces are not elided when defining a trait. The combination of a wrapping namespace and a qualified trait name always results in appending the namespace of the qualified trait to the wrapping namespace. For example:
```cpp
namespace first::second {

trait second::third::Trait { /* ...*/ }

}
```
This results in `first::second::second::third::Trait`, **not** `first::second::third::Trait`.
