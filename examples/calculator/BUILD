load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_test")
load("//bazel:cpp_module.bzl", "cpp_module")
load("//bazel:arc_gen.bzl", "arc_module_library")
load("//bazel:arc_instantiate.bzl", "arc_graph")

arc_module_library(
    name = "calc_lib",
    arcs = glob(["src/*.ixx.arc"]),
    mods = glob(["src/*.ixx", "src/nodes/*.ixx", "src/nodes/impl/*.ixx"]),
    srcs = glob(["src/*.cpp", "src/nodes/impl/*.cpp"], exclude = ["src/main.cpp"]),
)

GRAPH_NODES = {
    "repl": "examples.calculator.node.repl",
    "compute.evaluator": "examples.calculator.node.evaluator",
    "compute.functions": "examples.calculator.node.functions",
    "data.commands": "examples.calculator.node.command_handler",
    "data.persistence": "examples.calculator.node.file_persistence",
}

arc_graph(
    name = "batch_graph",
    graph_module = "examples.calculator.graphs",
    graph_type = "examples::calculator::graph::Batch",
    nodes = GRAPH_NODES,
)

arc_graph(
    name = "interactive_graph",
    graph_module = "examples.calculator.graphs",
    graph_type = "examples::calculator::graph::Interactive",
    nodes = GRAPH_NODES | {
        "lineReader": "examples.calculator.node.terminal_line_reader",
    },
)

cpp_module(
    name = "calc_main",
    srcs = ["src/main.cpp"],
    module_deps = [":calc_lib", ":batch_graph", ":interactive_graph"],
)

cc_binary(
    name = "arc_example_calculator",
    deps = [":calc_main"],
)

###########################################################################
# Tests
###########################################################################

arc_module_library(
    name = "calc_test_graphs",
    mods = ["tests/graphs.ixx"],
)

arc_graph(
    name = "test_functions_graph",
    graph_module = "examples.calculator.node.functions",
    graph_type = "arc::test::Graph<examples::calculator::node::Functions>",
    nodes = {"node": "examples.calculator.node.functions"},
)

arc_graph(
    name = "test_evaluator_graph",
    graph_module = "examples.calculator.node.evaluator",
    graph_type = "arc::test::Graph<examples::calculator::node::Evaluator>",
    nodes = {"node": "examples.calculator.node.evaluator"},
)

arc_graph(
    name = "test_repl_graph",
    graph_module = "examples.calculator.node.repl",
    graph_type = "arc::test::Graph<examples::calculator::node::Repl>",
    nodes = {"node": "examples.calculator.node.repl"},
)

arc_graph(
    name = "test_file_persistence_graph",
    graph_module = "examples.calculator.node.file_persistence",
    graph_type = "arc::test::Graph<examples::calculator::node::FilePersistence>",
    nodes = {"node": "examples.calculator.node.file_persistence"},
)

arc_graph(
    name = "test_command_handler_graph",
    graph_module = "examples.calculator.node.command_handler",
    graph_type = "arc::test::Graph<examples::calculator::node::CommandHandler>",
    nodes = {"node": "examples.calculator.node.command_handler"},
)

arc_graph(
    name = "integration_graph",
    graph_module = "examples.calculator.tests.graphs",
    graph_type = "examples::calculator::tests::IntegrationGraph",
    nodes = GRAPH_NODES,
)

cpp_module(
    name = "calc_test_srcs",
    srcs = glob(["tests/test_*.cpp"]),
    module_deps = [
        ":calc_lib",
        ":calc_test_graphs",
        ":batch_graph",
        ":test_functions_graph",
        ":test_evaluator_graph",
        ":test_file_persistence_graph",
        ":test_command_handler_graph",
        ":test_repl_graph",
        ":integration_graph",
    ],
    deps = ["@doctest//doctest:doctest"],
)

cc_test(
    name = "arc_example_calculator_tests",
    deps = [
        ":calc_test_srcs",
        "//tests:arc_tests_lib",
    ],
)
