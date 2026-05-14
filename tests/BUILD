load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bazel:cpp_module.bzl", "cpp_module")
load("//bazel:arc_gen.bzl", "arc_header_library", "arc_module")
load("//bazel:arc_instantiate.bzl", "arc_graph")

# Wrapper that exposes "doctest.h" (quote-include form) to all test targets
cc_library(
    name = "doctest",
    hdrs = ["include/doctest.h"],
    strip_include_prefix = "include",
    deps = ["@doctest//doctest:doctest"],
    defines = ["DOCTEST_CONFIG_SUPER_FAST_ASSERTS=1"],
    visibility = ["//visibility:public"],
)

# Test main library (provides doctest main)
cc_library(
    name = "arc_tests_lib",
    srcs = ["test_main.cpp"],
    deps = [":doctest"],
    visibility = ["//visibility:public"],
    alwayslink = True,
)

###########################################################################
# ABC Headers-based tests
###########################################################################

arc_header_library(
    name = "arc_tests_headers_lib",
    embed_arcs = {"test_embedded_headers.cpp": "arc/tests/test_embedded_headers.hxx"},
    visibility = ["//visibility:public"],
)

arc_header_library(
    name = "abc_headers_lib",
    arcs = ["abc/headers/include/abc/graph.hxx.arc"],
    hdrs = [
        "abc/headers/include/abc/alice.hpp",
        "abc/headers/include/abc/alice.tpp",
        "abc/headers/include/abc/bob.hpp",
        "abc/headers/include/abc/bob.tpp",
        "abc/headers/include/abc/charlie.hpp",
        "abc/headers/include/abc/charlie.tpp",
        "abc/headers/include/abc/ellie.hpp",
        "abc/headers/include/abc/graph_wrapper.hpp",
        "abc/headers/include/abc/traits.hpp",
    ],
    srcs = [
        "abc/headers/src/alice.cpp",
        "abc/headers/src/ellie.cpp",
    ],
    includes = ["abc/headers/include"],
    deps = ["//lib:arc_headers"],
)


arc_graph(
    name = "abc_src_lib",
    graph_header = "abc/graph.hxx",
    graph_type = "arc::Graph<abc::cluster::AliceBob>",
    nodes = {
        "alice": "abc/alice.tpp",
        "bob": "abc/bob.tpp",
        "charlie.charlie": "abc/charlie.tpp",
    },
    deps = [":abc_headers_lib", "//lib:arc_headers"],
)

cc_test(
    name = "arc_tests_headers",
    srcs = [
        "test_abc_headers.cpp",
        "test_embedded_headers.cpp",
        "test_mem_ptr.cpp",
    ],
    deps = [
        ":arc_tests_lib",
        ":abc_headers_lib",
        ":abc_src_lib",
        ":arc_tests_headers_lib",
        "//lib:arc_headers",
    ],
)

###########################################################################
# ABC Module-based library
###########################################################################

arc_module(
    name = "abc_lib",
    arcs = glob(["abc/modules/abc/**/*.ixx.arc"]),
    mods = glob(["abc/modules/abc/*.ixx"]),
    srcs = glob(["abc/modules/abc/*.cpp"]),
    deps = ["//lib:arc_headers"],
)

arc_graph(
    name = "abc_alice_graph",
    graph_module = "abc.graph",
    graph_type = "arc::Graph<abc::cluster::AliceBob>",
    nodes = {"alice": "abc.alice"},
)

###########################################################################
# Test modules (arc.tests.*)
###########################################################################

arc_module(
    name = "test_lib",
    arcs = [
        "test_domain/domain.ixx.arc",
        "test_repeater/traits.ixx.arc",
        "test_repeater/cluster.ixx.arc",
    ],
    mods = [
        "test_repeater/test_node.ixx",
        "test_datastore/types.ixx",
    ],
    embed_arcs = [
        "test_spy.cpp",
        "test_null_trait.cpp",
        "test_collection.cpp",
        "test_group.cpp",
        "test_embedded_modules.cpp",
        "test_global.cpp",
        "test_joined_trait.cpp",
        "test_node_count.cpp",
        "test_virtual.cpp",
        "test_contracts.cpp",
        "test_default_impl.cpp",
        "test_datastore/test_datastore.cpp",
        "test_datastore/test_single_listener.cpp",
        "test_protocol.cpp",
        "test_tracer_spy.cpp",
    ],
    deps = ["//lib:arc_headers", ":doctest"],
)

arc_module(
    name = "mock_mod",
    embed_arcs = ["test_mock.cpp"],
    deps = [":doctest"],
)

###########################################################################
# arc_tests_module test binary
###########################################################################

cpp_module(
    name = "module_test_srcs",
    srcs = ["test_repeater/main.cpp"] + glob(
        ["test_*.cpp", "test_datastore/*.cpp"],
        exclude = [
            "test_main.cpp",         # test framework entry point, not a test
            "test_abc_headers.cpp",  # arc_tests_headers
            "test_embedded_headers.cpp",
            "test_mem_ptr.cpp",
            "test_combine.cpp",      # arc_tests_mock
            "test_lazy.cpp",
            "test_mock.cpp",
        ],
    ),
    module_deps = [
        ":abc_lib",
        ":abc_alice_graph",
        ":test_lib",
    ],
    deps = [":doctest"],
)

cc_test(
    name = "arc_tests_module",
    deps = [
        ":module_test_srcs",
        ":arc_tests_lib",
    ],
)

###########################################################################
# arc_tests_mock test binary
###########################################################################

cpp_module(
    name = "mock_test_srcs",
    srcs = [
        "test_combine.cpp",
        "test_lazy.cpp",
        "test_mock.cpp",
    ],
    module_deps = [":mock_mod"],
    deps = [":doctest"],
)

cc_test(
    name = "arc_tests_mock",
    deps = [
        ":mock_test_srcs",
        ":arc_tests_lib",
    ],
)

###########################################################################
# arc_tests_thread test binary
###########################################################################

arc_module(
    name = "thread_lib",
    arcs = glob(["test_thread/*.ixx.arc"]),
    mods = glob(["test_thread/*.ixx"]),
    srcs = ["test_thread/poster.cpp"],
    embed_arcs = [
        "test_thread/test_collection.cpp",
        "test_thread/test_union.cpp",
    ],
    deps = [":doctest"],
)

cpp_module(
    name = "thread_test_srcs",
    srcs = glob(["test_thread/test_*.cpp"]),
    module_deps = [":thread_lib"],
    deps = [":doctest", "//lib:arc_headers"],
)

cc_test(
    name = "arc_tests_thread",
    deps = [
        ":thread_test_srcs",
        ":arc_tests_lib",
    ],
)
