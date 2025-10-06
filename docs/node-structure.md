# Node class structure

A Node is a C++ class that can implement its own traits and/or depend on traits implemented by other nodes.

## Nodes provide to other nodes:
1. List of implemented traits
2. The specific type that implements each trait's methods
3. The specific type that exports each trait's types

## Nodes are provided:
1. Access to implementations of its own traits, using the function `asTrait(trait::OtherTrait)`.
2. Access to other nodes by trait, using a `getNode(trait::Trait)` call, as long as the dependency is specified in the cluster definiton
3. Access to the types exported by other nodes by trait, using `arc::ResolveTypes<Node, trait::Trait>`, as long as the dependency is specified in the cluster definiton

## Defining a node class
There are two ways to define a node, each with different pros and cons:
1. More concise: inline node with contextless state

      ```cpp
      struct PiCache : arc::Node
      {
         // (Optional) list dependencies of this node
         using Depends = arc::Depends<trait::Pi>;
         // Checks early that the node has all required dependencies provided to it by the cluster.
         // If dependencies are specified, and any dependencies are queried via getNode or ResolveTypes
         // for traits not in the above list, the node will fail to compile.
         // If a requirement is optional, i.e. the getNode call is not necessarily made
         // by this node in every graph, then the trait can be specified as `trait::TraitName*`
         // in the Depends list, so that the dependency is only checked at the point of use.

         using Traits = arc::Traits<PiCache, trait::Pi>;

         // Context can only be deduced in member functions via an explicit object parameter (deducing-this)
         double impl(this auto& self, trait::Pi::calc)
         {
            if (std::isnan(self.piValue)) [[unlikely]]
               self.piValue = self.getNode(trait::pi).calc();
            return self.piValue;
         }

         double piValue = NAN;
      };
      ```

      - Best suited to a node that has few or no dependencies of its own:
         - Types **cannot** be resolved in the state from the node's injected context
         - Explicit object parameter is required in method implementations that have trait dependencies on other nodes
         - Methods with dependencies become template functions that must be defined directly in the header or module interface
2. More flexible: nested template node with contextful state

      ```cpp
      struct PiCache
      {
         // Nested node *must* be a unary class template called `Node`
         template<class Context>
         struct Node : arc::Node
         {
            using Traits = arc::Traits<Node, trait::Pi>;

            using Float = Context::Root::Float;

            // Context is available without needing to use an explicit object parameter
            Float impl(trait::Pi::calc)
            {
               if (std::isnan(piValue)) [[unlikely]]
                  piValue = getNode(trait::pi).calc();
               return piValue;
            }

            Float piValue = std::numeric_limits<Float>::quiet_NaN();
         };
      };
      ```

      - Types **can** be resolved in the state from the node's injected `Context`, which is the single template parameter of the node's state
      - Explicit object parameters are not necessary to resolve dependencies

      - Less methods with dependencies need to be template functions, allowing implementations to be defined in separate .cpp files
         <details>
         <summary>:eyes: Example: tpp header/cpp impl splitting of method declarations from definitions</summary>

         ```cpp
         // pi_cache.hpp
         #include <arc/arc.hpp>

         struct PiCache
         {
            // Nested node *must* be a unary class template called `Node`
            template<class Context>
            struct Node : arc::Node
            {
               using Traits = arc::Traits<Node, trait::Pi>;

               using Float = Context::Root::Float;

               Float impl(trait::Pi::calc);

               Float piValue = std::numeric_limits<Float>::quiet_NaN();
            };
         };
         ```
         ```cpp
         // pi_cache.tpp
         #include "pi_cache.hpp"

         template<class Context>
         auto PiCache::Node<Context>::impl(trait::Pi::Calc) -> Float
         {
            if (std::isnan(piValue)) [[unlikely]]
               piValue = getNode(trait::pi).calc();
            return piValue;
         }
         ```
         ```CMake
         # CMakeLists.txt
         add_executable(math_lib main.cpp)

         # Generates math.hxx from math.hxx.arc (which defines MathCluster)
         target_generate_arc_headers(math_lib)

         # Generates {build_dir}/src/piCache.cpp, which instantiates PiCache::Node<Context>
         # with the Context injected by MathCluster, and adds it to math_lib
         target_generate_arc_src(math_lib
            GRAPH_HEADER   math.hxx
            GRAPH_TYPE     arc::Graph<MathCluster>
            NODES
               piCache     pi_cache.tpp
               # ... other nodes not shown here
         )
         ```
         ```cpp
         // {build_dir}/src/piCache.cpp generated by target_generate_arc_src
         #include "math.hxx"
         #include <arc/macros.hpp>

         #include "pi_cache.tpp"

         ARC_INSTANTIATE(arc::Graph<MathCluster>, piCache)
         ```
         </details>

## Listing implemented traits

All nodes must define a nested `Traits` type, which provides the mapping of each trait to its methods and types in the node.

ARC provides a template to define this mapping in the form `arc::Types<{Node}, {TraitMapping}...>`

|`{Node}` kind|`{State}`: node state type|`{DefaultImpl}`: default methods type|`{DefaultTypes}`: default types type|
|:---|:---|:---|:---|
|`State`|`State`|`State`|`State::Types`|
|`State(Impl)`|`State`|`Impl`|`State::Types`|

|`{TraitMapping}` kind|`{Trait}`|`{Impl}`: methods type|`{Types}`: types type|
|:---|:---|:---|:---|
|`trait::Trait`|`trait::Trait`|`{DefaultImpl}`|`{DefaultTypes}`|
|`trait::Trait(TraitImpl)`|`trait::Trait`|`TraitImpl`|`{DefaultTypes}`|
|`trait::Trait*(TraitTypes)`|`trait::Trait`|`{DefaultImpl}`|`TraitTypes`|
|`trait::Trait(TraitImpl, TraitTypes)`|`trait::Trait`|`TraitImpl`|`TraitTypes`|

Most nodes will have a flat structure, with `Traits` in the shorthand form `arc::Traits<Node, trait::Trait1, trait::Trait2>`, i.e. `arc::Traits<{State}, {Trait}...>`. Using `{Impl} = {DefaultImpl} = {State}` and `{Types} = {DefaultTypes} = {State}::Types` for methods and types of all traits is usually sufficient unless the node is particularly complex.

<details>
<summary>:eyes: Code: Exhaustive arc::Traits example</summary>

```
cluster Dinner
{
   guest = Guest
   fruit = FruitBasket

   using g = trait::Guest
   using a = trait::Apple, b = trait::Banana, o = trait::Orange, t = trait::Tangerine

   [g] .. --> guest
   [a]        guest --> fruit
   [b]        guest --> fruit
   [o]        guest --> fruit
   [t]        guest --> fruit
}

trait Guest
{
   eat() -> void
}

trait Apple
{
   take()
}

trait Banana
{
   take()
}

trait Orange [Types]
{
   requires typename Types::Orange
   take() -> Types::Orange
}

// Tangerine has an identical interface to Orange, but is resolved by a different name (advanced)
trait Tangerine = Orange
```
```cpp
struct FruitBasket
{
   template<class Context>
   struct Node : arc::Node
   {
      struct Apple; // detached interface
      struct BananaTypes;
      struct Tangerine; // overrides Node::impl(trait::Orange::take)
      struct TangerineTypes; // overrides Types::Orange

      using Traits = arc::Traits<Node
         , trait::Orange // methods: Node, types: Node::Types
         , trait::Apple(Apple)  // m: Apple, t: Node::Types
         , trait::Banana*(BananaTypes) // m: Node, t: BananaTypes
         , trait::Tangerine(Tangerine, TangerineTypes) // m: Tangerine, t: TangerineTypes
      >;

      struct Types
      {
         using Apple = int;
         using Orange = int;
      };

      struct TangerineTypes
      {
         using Orange = long;
      };

      struct BananaTypes
      {
         using Banana = int;
      };

      int apples = 2, oranges = 1, bananas = 2;
      long tangerines = 5;

      template<class Int>
      static Int take(Int& source, Int amount)
      {
         Int taken = std::min(source, amount);
         source -= taken;
         return taken;
      }

      Types::Orange impl(trait::Orange::take, Types::Orange amount)
      {
         return take(oranges, amount);
      }

      struct Apple : arc::DetachedInterface
      {
         // Detached interface accesses state through explicit object paramater
         Types::Apple impl(this auto& self, trait::Apple::take, Types::Apple amount)
         {
            return take(self->apples, amount);
         }
      };

      BananaTypes::Banana impl(trait::Banana::take, BananaTypes::Banana amount)
      {
         return take(bananas, amount);
      }
   };
};

// Tangerine extends Node, so must be defined after Node
template<class Context>
struct FruitBasket::Node<Context>::Tangerine : Node
{
   // Overrides Node::impl(trait::Orange::take, int) for trait::Tangerine only
   TangerineTypes::Orange impl(trait::Orange::take, TangerineTypes::Orange amount)
   {
      return take(tangerines, amount);
   }
};
```
```cpp
struct Guest : arc::Node
{
   using Traits = arc::Traits<Guest, trait::Guest>;

   template<class Self>
   void impl(this Self& self, trait::Guest::eat)
   {
      // FruitBasket::Node<...>::Types::Orange
      using Orange = arc::ResolveTypes<Self, trait::Orange>::Orange;
      // FruitBasket::Node<...>::Types::Apple
      using Apple = arc::ResolveTypes<Self, trait::Apple>::Apple;
      // FruitBasket::Node<...>::BananaTypes::Banana
      using Banana = arc::ResolveTypes<Self, trait::Banana>::Banana;
      // FruitBasket::Node<...>::TangerineTypes::Orange
      using Tangerine = arc::ResolveTypes<Self, trait::Tangerine>::Orange;

      // FruitBasket::Node<...>::impl(trait::Orange::take{}, 3)
      std::same_as<Orange> auto oranges = self.getNode(trait::orange).take(3);
      // arc::DetachedImpl<FruitBasket::Node<...>, FruitBasket::Node<...>::Apple>::impl(trait::Apple::take{}, 3)
      std::same_as<Apple> auto apples = self.getNode(trait::apple).take(3);
      // FruitBasket::Node<...>::impl(trait::Banana::take{}, 3)
      std::same_as<Banana> auto bananas = self.getNode(trait::banana).take(3);
      // FruitBasket::Node<...>::Tangerine::impl(trait::Orange::take{}, 3)
      std::same_as<Tangerine> auto tangerines = self.getNode(trait::tangerine).take(3);

      assert(apples == 2);
      assert(oranges == 1);
      assert(bananas == 2);
      assert(tangerines == 3);
   }
};
```
</details>

## Implicit traits

It is possible for a node to implicitly implement traits, by using `arc::TraitsOpen<{Node}, {TraitMapping}...>`, which uses `{DefaultImpl}` and `{DefaultTypes}` for all traits not explicitly mapped.
