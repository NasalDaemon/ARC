# Embedding cluster and trait DSL into existing source files

Clusters and traits can be defined via the ARC DSL (aka arc) embedded and interleaved into existing source files (e.g., [modules](../tests/test_embedded_modules.cpp), [headers](../tests/test_embedded_headers.cpp)), or via separate .arc files. See [cluster](cluster-syntax.md) and [trait](trait-syntax.md) syntax for the overall syntax.

Embedding arc into existing source files is useful when writing tests, as defining single-use traits and clusters in separate source files can clutter the source directory and increase the distance between the tests and the definitions of the entities they use.

To interleave a arc into another source file, introduce a section of the interleaved arc using the keywords `arc-embed-begin` and `arc-embed-end`. This is best achieved via comments. Ensure that the section is not compiled in the hosting source file, as the arc syntax is not C++.

```cpp
module;
#include <doctest/doctest.h>
module my.test.clusters;

#if 0 // ensures that the snippet is not compiled as C++
// arc-embed-begin
export module my.test.clusters;

cluster my::test::Cluster1
{
    // ...
}
// arc-embed-end
#endif

TEST_CASE("first")
{
    // using my::test::Cluster1
}

// Multi-line comments are also possible
/* arc-embed-begin
cluster my::test::Cluster2
{
    // ...
}
arc-embed-end */

TEST_CASE("second")
{
    // using my::test::Cluster2
}
```

The interleaved arc can be spread out over multiple sections, all wrapped with the same keywords `arc-embed-begin` and `arc-embed-end`. The example above results in the following intermediate arc by stitching together the sections:

```cpp
export module my.test.clusters;

cluster my::test::Cluster1
{
    // ...
}

cluster my::test::Cluster2
{
    // ...
}
```

This will then be used to generate the .ixx module that is compiled.
