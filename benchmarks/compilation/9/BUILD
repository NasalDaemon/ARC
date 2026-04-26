load("@rules_cc//cc:defs.bzl", "cc_test")
load("//bazel:cpp_module.bzl", "cc_module_bundle")
load("//bazel:arc_gen.bzl", "arc_module_library")
load("//bazel:arc_instantiate.bzl", "arc_graph")

arc_module_library(
    name = "bench9_lib",
    arcs = glob(["traits/*.ixx.arc", "*.ixx.arc"]),
    mods = glob(["nodes/*.ixx", "nodes/*.impl.ixx"]),
    tags = ["manual"],
)

arc_graph(
    name = "bench9_graph",
    graph_module = "arc.bench.compile9.graph",
    graph_type = "arc::Graph<arc::bench::compile9::cluster::Graph>",
    nodes = {
        "node1": "arc.bench.compile9.node1",
        "node2": "arc.bench.compile9.node2",
        "node3": "arc.bench.compile9.node3",
        "node4": "arc.bench.compile9.node4",
        "node5": "arc.bench.compile9.node5",
        "node6": "arc.bench.compile9.node6",
        "node7": "arc.bench.compile9.node7",
        "node8": "arc.bench.compile9.node8",
        "node9": "arc.bench.compile9.node9",
    },
    tags = ["manual"],
)

cc_module_bundle(
    name = "bench9_main",
    srcs = ["main.cpp"],
    module_deps = [":bench9_lib", ":bench9_graph"],
    tags = ["manual"],
)

cc_test(
    name = "arc_bench_compile9",
    deps = [":bench9_main"],
    tags = ["manual"],
)
