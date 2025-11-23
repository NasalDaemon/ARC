# Selective Runtime Polymorphism

In large-scale C++ systems, balancing static structure with runtime flexibility is crucial—especially when migrating from a modular monolith toward microservices, or when you need to swap implementations on the fly (for testing, mocking, or hot-reloading). Traditional C++ dependency injection is either fully static (compile-time) or fully dynamic (runtime), but rarely both in a controlled, type-safe way.

`arc::Union` and `arc::Virtual` exist to bridge this gap. They let you introduce runtime polymorphism *only where you need it*, keeping the rest of your dependency graph static and efficient. With these tools, you can:

- **Select implementations at runtime** (e.g., for plugins, mocks, or feature toggles)
- **Hot-swap nodes** without restarting your application
- **Compose graphs that are mostly static** (for performance and clarity), but with dynamic "seams" where flexibility is required

This selective approach enables you to build systems that are both robust and adaptable—well-suited for architectures that need to scale from monoliths to microservices, or that require live reconfiguration and testing without sacrificing type safety or performance.

## `arc::Union`

`arc::Union<Node, Nodes...>` (see [union.hpp](../lib/include/arc/union.hpp)) is to be used if the full set of possible nodes implementing an intersection of traits is known at compile-time. When the graph is constructed, a single implementation is constructed. This remains active until an alternative implementation from the list is emplaced during runtime, or the graph is destructed.

Nodes hosted in a `arc::Union` can swap themselves with another alternative from the list by calling `auto deferredExchange = exchangeImpl<NewImpl>(constructorArgs...)` in the active node (arguments are taken by value). When `deferredExchange` is destroyed, it replaces the current option with `NewImpl`, invalidating the current node state. You can also swap from outside the graph using `graph.node->emplace<Index>(...)` or `graph.node->emplace<Type>(...)`. **NOTE:** Unlike `arc::Virtual`, `arc::Union` does not provide a detached handover; the lifetimes of the old and new alternative do not overlap.

The memory layout of `arc::Union<Nodes...>` is much like `std::variant<Ts...>`, where the state of the active alternative is stored on the stack alongside its respective index. Since only one node alternative is active at any one time, a `union` is effectively used to host the state. When a trait method is invoked, the only extra runtime cost is a branch to pick the active node's implementation given the active index. Due to the list of alternatives being known at compile-time, it presents opportunities for the compiler to optimise invocations, including inlining. Nodes in a `arc::Union` are also able to retain compile-time thread-affinity protection where it is employed.

Only method calls from external nodes into the `arc::Union` incur the extra runtime cost. Any methods invoked from within the nodes hosted by the `arc::Union` incur no extra runtime cost, including calls to methods external to the `arc::Union`.

### Examples ([examples/animals](../examples/animals))

```cpp
export module app.cluster;

import app.traits;
import app.farmer; // not shown
export import app.cow;
export import app.sheep;

cluster app::Cluster
{
    farmer = Farmer
    animal = arc::Union<Cow, Sheep>

    [trait::Animal]
    farmer --> animal
}
```
```cpp
export module app.traits;

import std;

trait app::trait::Animal
{
    speak() const -> std::string
}
```
```cpp
export module app.cow;

import app.traits;
import arc;

namespace app {

export struct Cow : arc::Node
{
    using Traits = arc::Traits<Cow, trait::Animal>;

    std::string impl(trait::Animal::speak) const { return happy ? "moo" : "mmmooooo!"; }

    explicit Cow(bool happy) : happy(happy) {}
    bool happy;
};

}
```
```cpp
export module app.sheep;

import app.traits;
import arc;

namespace app {

export struct Sheep : arc::Node
{
    using Traits = arc::Traits<Sheep, trait::Animal>;

    std::string impl(trait::Animal::speak) const { return "baa!"; }
};

}
```

```cpp
import app.cluster;
import std;

enum Animal
{
    Cow, Sheep
};

struct Args
{
    Animal animal;
};

Args parseArgs(int argc, char** argv);

int main(int argc, char** argv)
{
    Args args = parseArgs(argc, argv);

    arc::Graph<app::Cluster> graph{
        .animal{arc::withFactory,
            [&](auto construct) {
                if (args.animal == Animal::Cow)
                    return construct(std::in_place_type<Cow>, true);
                else
                    // Can use index
                    return construct(std::in_place_index<1 /* Sheep */>);
            }}
    };

    std::println("Animal says {}", graph.farmer->getNode(trait::animal).speak());

    // Change impl at runtime by index
    graph.animal->emplace<0>(false);
    std::println("Unhappy cow says {}", graph.farmer->getNode(trait::animal).speak());
    // Change impl at runtime by type
    graph.animal->emplace<Sheep>();
    std::println("Sheep says {}", graph.farmer->getNode(trait::animal).speak());
}
```

## `arc::Virtual`

`arc::Virtual<Interfaces...>` (see [virtual.hpp](../lib/include/arc/virtual.hpp)) is to be used to host an open-set of node implementations of the given `Interfaces` (which derive from `arc::INode`). This is the most flexible option, as any implementation of the interfaces can be used, even if it is not known when compiling the graph. As with `arc::Union` when the graph is constructed, a single implementation is constructed. This remains active until an alternative implementation of the interface is emplaced during runtime, or the graph is destructed.

Nodes deriving from `arc::INode` have the ability to swap themselves out with another implementation and handover interactively as they see fit by using `auto handle = exchangeImpl<NewImpl>(constructorArgs...)`. The `handle` obtains ownership over the calling node's detached state—which can reach the rest of the graph, but cannot itself be reached from other nodes. It also holds a non-owning reference to the new implementation which has been attached to the graph in its stead, allowing for an interactive handover.

The memory layout of `arc::Virtual<Interfaces...>` is much like `std::tuple<Interfaces*...>`, where the `Interfaces*` are pointers to the heap-allocated implementation. When a trait method is invoked, the interface that satisfies the trait is found from the list (as long as there is no ambiguity) and the method is called on that interface. Since the trait methods are typically implemented as virtual overrides of the interface methods, one vtable lookup is to be expected per method call, much like a standard abstract class. In the case of a virtual override, it is harder for the compiler to optimise the call or inline it, although with judicious use of the `final` keyword and with whole-program or link-time-optimisation it may be possible for the compiler to devirtualise the call. Nodes hosted in a `arc::Virtual` are not able to retain compile-time thread-affinity protection where it is employed, as the types needed to enforce this are erased.

Only method calls from external nodes into nodes hosted by `arc::Virtual` incur the extra runtime cost. Any methods invoked from within the nodes hosted by the `arc::Virtual` incur no extra runtime cost, including calls to methods external to the `arc::Virtual`.

**NOTE:** It is possible to make a `arc::Virtual` one of the alternatives in a `arc::Union`, offering the best of both worlds. One can specify the static nodes that are most well-known or requiring the highest performance as alternatives in the `arc::Union`, and the `arc::Virtual` alternative can be used as a fallback for everything else. Invoking a method on the fallback `arc::Virtual` would then of course require both a branch (`arc::Union` active node selection) as well as a vtable lookup (`arc::Virtual` method dispatch).

### `arc::Adapt` and `arc::Box`: hosting static nodes in a `arc::Virtual`

By using `arc::Adapt<StaticNode, InterfaceFacade>`, it is possible to adapt a static node that doesn't derive from `arc::INode` or use virtual overrides, so that it can be hosted by a `arc::Virtual`. The second template parameter is a facade, implementing the `Interface` required by the hosting `arc::Virtual<Interface>`, which invokes the respective trait methods on the static node like a proxy. The static node, being static and having no knowledge of being dynamic, is not capable of swapping itself out, but the facade can swap the implementation of the `arc::Virtual`, as it is a dynamic node. The static node can still call methods external to `arc::Virtual` as per usual with no extra vtable lookup.

With `arc::Box<StaticNode, InFacade, OutFacade = void, OutInterfaces...>`, one adapts the given static node but also adapts all external dependencies behind another `OutFacade`, implementing `OutInterfaces...`. This means that the static node must pay a virtual dispatch also in all its calls to its dependencies, which are entirely abstracted behind the `OutFacade`. Using `arc::Box`, one can instantiate a completely isolated implementation of the interfaces from which `InFacade` derives, completely agnostic to the context of the hosting `arc::Virtual`.

### Examples ([examples/animals_virtual](../examples/animals_virtual))

```cpp
export module app.cluster;

import app.traits;
import app.farmer; // not shown
export import app.animal;

cluster app::Cluster
{
    farmer = Farmer
    animal = arc::Virtual<IAnimal>

    [trait::Animal]
    farmer --> animal
}
```
```cpp
export module app.traits;

import std;

trait app::trait::Animal
{
    speak() const -> std::string
    evolve()
}
```
```cpp
export module app.animal;

import app.traits;
import arc;
import std;

namespace app {

struct IAnimal : arc::INode
{
    using Traits = arc::Traits<IAnimal, trait::Animal>;

    virtual std::string impl(trait::Animal::speak) const = 0;
    virtual void impl(trait::Animal::evolve) const = 0;
};

}
```
```cpp
export module app.cow;

import app.animal;
import app.traits;
import arc;

namespace app {

export struct Cow
{
    template<class Context>
    struct Node final : IAnimal
    {
        using Traits = arc::TraitsFrom<Node, IAnimal>;

        std::string impl(trait::Animal::speak) const override
        {
            return happy ? "moo" : "mmmooooo!";
        }

        void impl(trait::Animal::evolve) override
        {
            // Cows do not evolve
        }

        explicit Node(bool happy) : happy(happy) {}
        bool happy;
    };
};

}
```
```cpp
export module app.sheep;

import app.animal;
import app.traits;
import app.goat;
import arc;

namespace app {

export struct Sheep {
    template<class Context>
    struct Node final : IAnimal
    {
        using Traits = arc::TraitsFrom<Node, IAnimal>;

        std::string impl(trait::Animal::speak) const override
        {
            return black ? "barbar" : "baa!";
        }

        void impl(trait::Animal::evolve) override
        {
            auto handle = exchangeImpl<Goat>();
            // In the context of a global scheduler, this exchange will be deferred until
            // all scheduler tasks have been drained and the scheduler is in exclusive mode (idle).
            // For the sake of simplicity, we assume there is no global scheduler here.
            if (handle.deferred())
                throw std::runtime_error("Sheep::evolve exchangeImpl was not expected to be deferred");

            // This node is still alive in a detached state
            // It can call into itself and other nodes, but other nodes can't reach it
            std::println("{}", asTrait(trait::animal).speak());
            // prints: barbar or baa!

            std::println("{}", handle.getNext()->asTrait(trait::animal).speak());
            // prints: meeh!
        }

        Node(bool black = false) : black(black) {}
        bool black;
    };
};

}
```
```cpp
export module app.goat;

import app.animal;
import app.traits;
import app.cow;
import arc;

namespace app {

export struct Goat {
    template<class Context>
    struct Node final : IAnimal
    {
        using Traits = arc::TraitsFrom<Node, IAnimal>;

        std::string impl(trait::Animal::speak) const override { return "meeh!"; }

        void impl(trait::Animal::evolve) override
        {
            auto handle = exchangeImpl<Cow>(false);
            if (handle.deferred())
                throw std::runtime_error("Goat::evolve exchangeImpl was not expected to be deferred");

            // This node is still alive in a detached state
            // It can call into itself and other nodes, but other nodes can't reach it
            std::println("{}", asTrait(trait::animal).speak());
            // prints: meeh!

            std::println("{}", handle.getNext()->asTrait(trait::animal).speak());
            // prints: mmmooooo!
        }
    };
};

}
```
```cpp
export module app.fox;

import app.traits;
import arc;

namespace app {

// Fox is a static node which we can adapt to IAnimal using arc::Adapt<Fox, IAnimalFacade>
export struct Fox : arc::Node
{
    using Traits = arc::Traits<Fox, trait::Animal>;

    std::string impl(trait::Animal::speak) const override { return "yip"; }

    void impl(trait::Animal::evolve)
    {
        // static node cannot swap itself out inside a arc::Virtual (alas, `evolve` shouldn't really be a method in the trait)
    }
};

}
```
```cpp
export module app.animal.facade;

import app.animal;
import app.cow;
import app.traits;
import arc;
import std;

namespace app {

export struct IAnimalFacade
{
    template<class Context>
    struct Node final : IAnimal
    {
        using Traits = arc::TraitsFrom<Node, IAnimal>;

        // Forward to adapted node
        std::string impl(trait::Animal::speak) const override
        {
            return getNode(trait::animal).speak();
        }

        // As a dynamic node, this facade can swap the implementation hosted by arc::Virtual
        void impl(trait::Animal::evolve) override
        {
            auto handle = exchangeImpl<Cow>(true);
            if (handle.deferred())
                throw std::runtime_error("IAnimalFacade::evolve exchangeImpl was not expected to be deferred");

            // This node is still alive in a detached state
            // It can call into itself and other nodes, but other nodes can't reach it
            std::println("{}", asTrait(trait::animal).speak());
            // prints: yip! (assuming it adapted Fox)

            std::println("{}", handle.getNext()->asTrait(trait::animal).speak());
            // prints: moo!
        }
    };
};

}
```
```cpp
import app.animal.facade;
import app.cluster;
import app.cow;
import app.sheep;
import app.fox;
import app.traits;
import std;

enum class Animal
{
    Cow, Sheep
};

struct Args
{
    Animal animal;
};

Args parseArgs(int argc, char** argv);

using namespace app;

int main(int argc, char** argv)
{
    Args args = parseArgs(argc, argv);

    arc::Graph<app::Cluster> graph{
        .animal{arc::withFactory,
            [&](auto construct) {
                if (args.animal == Animal::Cow)
                    return construct(std::in_place_type<Cow>, true);
                else
                    return construct(std::in_place_type<Sheep>);
            }}
    };

    auto animal = graph.farmer.getNode(trait::animal); // equivalent to graph.animal.asTrait(trait::animal)
    std::println("Animal says {}", animal.speak());

    graph.animal->emplace<Sheep>(true);
    std::println("Black sheep says {}", animal.speak());

    // Sheep hotswaps with goat
    animal.evolve();
    std::println("Goat says {}", animal.speak());
    // Goat hotswaps with sad cow
    animal.evolve();
    std::println("Sad cow says {}", animal.speak());

    // Farmer's view of animal is always up-to-date
    // The `animal` variable is just a non-owning trait view of the arc::Virtual node in the graph
    std::println("Sad cow says {}", graph.farmer.getNode(trait::animal).speak());

    graph.animal->emplace<arc::Adapt<Fox, IAnimalFacade>>();
    std::println("Fox says {}", animal.speak());
    // IAnimalFacade hot-swaps with happy cow
    animal.evolve();
    std::println("Cow says {}", animal.speak());
}
```
