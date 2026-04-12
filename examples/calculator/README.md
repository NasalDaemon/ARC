# Calculator Example

A REPL calculator built with ARC, demonstrating traits, clusters, graphs, and node composition.

## Features

- Arithmetic: `+`, `-`, `*`, `/`, `^` with operator precedence and parentheses
- Variables: `x = 3 + 1`, then use `x` in later expressions; `ans` holds the last result
- User-defined functions with arity-based overloading: `f(x) = x^2` and `f(x, y) = x^2 + y` coexist
- Built-in functions: `abs`, `sqrt`, `neg`, `sin`, `cos`, `tan`, `log` (base 10), `ln`, `min`, `max`, `add`, `sub`, `mul`, `div`, `pow`
- Commands: `help`, `vars`, `fns`, `history`, `clear`, `undef`, `save`, `load`
- State persistence: save and load variables and user functions to a file
- Two modes: interactive (terminal with history navigation) and batch (CLI arguments)

## Usage

```
# Interactive mode
arc_example_calculator

# Batch mode — each argument is evaluated in order
arc_example_calculator "x = 5" "x^2 + 1"
```

## Examples

### Arithmetic and precedence

```
> 2 + 3 * 4
14
> (2 + 3) * 4
20
> 2 ^ 10
1024
> -3 + 7
4
```

### Variables and `ans`

```
> x = 5
x = 5
> y = x * 2 + 1
y = 11
> x + y
= 16
> ans / 4
= 4
```

### Built-in functions

```
> sqrt(144)
12
> sin(3.14159 / 2)
1
> log(1000)
3
> ln(1)
0
> max(3, 7) + min(3, 7)
10
> abs(-42)
42
> neg(5)
-5
```

### User-defined functions

Functions are resolved by name and arity, so different arities of the same name coexist independently:

```
> f(x) = x^2 + 1
Defined f(x)
> f(5)
26
> f(x, y) = x^2 + y
Defined f(x, y)
> f(5)
26
> f(3, 7)
16
> hyp(a, b) = sqrt(a^2 + b^2)
Defined hyp(a, b)
> hyp(3, 4)
5
```

### Commands

```
> vars
  x = 5
  y = 11

> fns
Built-in: abs, add, cos, div, ln, log, max, min, mul, neg, pow, sin, sqrt, sub, tan
f(x) = x^2 + 1
f(x, y) = x^2 + y
hyp(a, b) = sqrt(a^2 + b^2)

> save mystate
Saved to mystate

> clear
Variables and user functions cleared.

> load mystate
Loaded from mystate

> undef f
Function(s) removed.

> history
  1  x = 5
  2  y = x * 2 + 1
  ...
```

### Batch mode

```bash
$ arc_example_calculator "radius = 3" "pi = 3.14159" "area = pi * radius^2" "area"
radius = 3
pi = 3.14159
area = 28.2743
= 28.2743
```

## Architecture

### Traits

Defined in [`src/traits.ixx.arc`](src/traits.ixx.arc) using the ARC DSL. Each trait is a named interface with preconditions and postconditions:

| Trait | Role |
|---|---|
| `Tokeniser` | Splits input string into tokens |
| `Parser` | Builds an AST from tokens |
| `Evaluator` | Evaluates an AST, producing a result or error |
| `Variables` | Named variable storage (get/set/remove/list/clear) |
| `BuiltinFunctions` | Dispatch and listing of built-in functions |
| `UserFunctions` | User-defined function registry (define/get/remove/clear by name+arity) |
| `Functions` | Combined alias: `BuiltinFunctions + UserFunctions` |
| `Formatter` | Formats results, errors, and listings for display |
| `LineReader` | Abstracts line input (terminal vs. batch) |
| `Output` | Abstracts text output |
| `History` | Expression history storage |
| `Commands` | Dispatches REPL commands (`help`, `vars`, etc.) |
| `Persistence` | Save/load calculator state to/from a file |

### Cluster

[`src/clusters.ixx.arc`](src/clusters.ixx.arc) defines the `Calculator` cluster, which wires all nodes together and declares which node sees which trait. The cluster is parameterised by a `Root` type that supplies `LineReader` and `Output` implementations, enabling mode switching without changing the core logic.

### Graphs

[`src/graphs.ixx`](src/graphs.ixx) defines two concrete graphs:

- **`Interactive`** — uses `TerminalLineReader` (readline-style input with history navigation) and `ConsoleOutput`.
- **`Batch`** — uses `BatchLineReader` (reads from a pre-set list of strings) and `ConsoleOutput`.

### Nodes

Each node lives in `src/nodes/` with its interface in an `.ixx` file and its implementation in `impl/`. Nodes with context dependencies use the `.impl.ixx` pattern for parallel compilation.
