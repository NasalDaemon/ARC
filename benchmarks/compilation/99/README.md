# Compilation Benchmark 99

This benchmark evaluates the compiler's ability to optimize across a large number of C++20 module translation units.

## Structure and Purpose

The benchmark consists of a chain of 99 nodes, each defined in its own module translation unit (100 TUs in total including `main`). Each node calls the next one in the sequence.

The purpose is twofold:
1. **Optimization Verification**: It demonstrates that the compiler can successfully inline and collapse the entire 99-node call chain. As shown in the assembly below, the entire program is reduced to a simple `return 0;` in `main` when Link Time Optimization (LTO) is enabled in a release build.
2. **Incremental Build Performance**: It serves as a test case for measuring the speed of incremental development when working with a deep hierarchy of module dependencies.

## Resulting Assembly

```c++
int main()
{
    arc::Graph<Graph> g;
    // Calls through a chain of 99 nodes
    if (g.node99.asTrait(trait::trait99).get() == 4950)
        return 0;
    return 1;
}
```

results in assembly (from `objdump -d`):

```asm
xor    %eax,%eax
ret
```

which is equivalent to:

```c
int main()
{
    return 0;
}
```
