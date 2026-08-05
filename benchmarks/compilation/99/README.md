# Compilation Benchmark 99

This benchmark evaluates the compiler's ability to optimize across a large number of ARC nodes in separate C++20 module translation units.

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

## Automated Check

This property is enforced by the ctest test `arc_bench_compile99_collapsed_asm`
(and `arc_bench_compile99_collapsed_asm` for the single-TU variant in
[99_seq](../99_seq)), added only when `ARC_BUILD_LTO` is enabled. The tests run
`objdump` over `main` and fail if any call or external tail-call survives the
optimiser, or if `main` exceeds a few instructions. They are added in Release
configurations only (multi-config generators skip other configurations at test
time via [check_collapsed_main.cmake](../check_collapsed_main.cmake)).
