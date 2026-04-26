load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")
load("//bazel:cpp_module.bzl", "cc_module_bundle")
load("//bazel:arc_gen.bzl", "arc_module_library")
load("//bazel:arc_instantiate.bzl", "arc_graph")

arc_module_library(
    name = "fs_lib",
    arcs = glob(["*.ixx.arc"]),
    mods = glob(["*.ixx", "nodes/*.ixx", "nodes/impl/*.ixx"]),
    srcs = glob(["nodes/impl/*.cpp"]),
)

arc_graph(
    name = "in_memory_repl_graph",
    graph_module = "examples.filesystem.graphs",
    graph_type = "examples::filesystem::graph::InMemoryRepl",
    nodes = {
        "repl": "examples.filesystem.repl",
        "fs.fs": "examples.filesystem.filesystem",
    },
)

arc_graph(
    name = "disk_repl_graph",
    graph_module = "examples.filesystem.graphs",
    graph_type = "examples::filesystem::graph::DiskRepl",
    nodes = {
        "repl": "examples.filesystem.repl",
        "fs.fs": "examples.filesystem.filesystem",
    },
)

cc_module_bundle(
    name = "fs_in_memory_repl",
    srcs = ["in_memory_repl.cpp"],
    module_deps = [":fs_lib", ":in_memory_repl_graph"],
)

cc_binary(
    name = "arc_example_filesystem_in_memory_repl",
    deps = [":fs_in_memory_repl"],
)

cc_module_bundle(
    name = "fs_disk_repl",
    srcs = ["disk_repl.cpp"],
    module_deps = [":fs_lib", ":disk_repl_graph"],
)

cc_binary(
    name = "arc_example_filesystem_disk_repl",
    deps = [":fs_disk_repl"],
)

###########################################################################
# Tests
###########################################################################

arc_module_library(
    name = "fs_test_mocks",
    mods = ["tests/mocks.ixx"],
)

arc_graph(
    name = "in_memory_graph",
    graph_module = "examples.filesystem.graphs",
    graph_type = "examples::filesystem::graph::InMemory",
    nodes = {
        "fs": "examples.filesystem.filesystem",
    },
)

arc_graph(
    name = "test_filesystem_graph",
    graph_module = "examples.filesystem.tests.mocks",
    graph_type = "arc::test::Graph<examples::filesystem::node::Filesystem, arc::test::Mock<examples::filesystem::tests::MockStorageTypes>>",
    nodes = {
        "node": "examples.filesystem.filesystem",
    },
)

cc_module_bundle(
    name = "fs_test_srcs",
    srcs = glob(["tests/test_*.cpp"]),
    module_deps = [
        ":fs_lib",
        ":fs_test_mocks",
        ":in_memory_graph",
        ":test_filesystem_graph",
    ],
    deps = ["@doctest//doctest:doctest"],
)

cc_test(
    name = "arc_example_filesystem_tests",
    deps = [
        ":fs_test_srcs",
        "//tests:arc_tests_lib",
    ],
)
