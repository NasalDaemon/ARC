load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")
load("//bazel:cpp_module.bzl", "cpp_module_test")
load("//bazel:arc_gen.bzl", "arc_header_library", "arc_module")
load("//bazel:arc_instantiate.bzl", "arc_graph")
load("//bazel:compile_fail.bzl", "compile_fail_test")

###########################################################################
# ABC Headers-based tests
###########################################################################

arc_header_library(
    name = "arc_tests_headers_lib",
    embed_arcs = {
        "test_embedded_headers.cpp": "arc/tests/test_embedded_headers.hxx",
        "test_nodiscard.cpp": "arc/tests/test_nodiscard.hxx",
        "test_trunk.cpp": "arc/tests/test_trunk.hxx",
    },
    visibility = ["//visibility:public"],
    testonly = True,
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
    testonly = True,
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
    deps = [":abc_headers_lib"],
    testonly = True,
)

cc_test(
    name = "arc_tests_headers",
    srcs = [
        "test_abc_headers.cpp",
        "test_embedded_headers.cpp",
        "test_mem_ptr.cpp",
        "test_nodiscard.cpp",
        "test_trunk.cpp",
    ],
    deps = [
        "//doctest:doctest",
        ":abc_headers_lib",
        ":abc_src_lib",
        ":arc_tests_headers_lib",
    ],
)

# Discarding a [[nodiscard]] trait method's result fails the compile on each call surface
compile_fail_test(
    name = "nodiscard_discards",
    src = "test_nodiscard.cpp",
    deps = [
        "//doctest:doctest",
        ":arc_tests_headers_lib",
    ],
    cases = [
        "ARC_DISCARD_NAMED",
        "ARC_DISCARD_OVERLOAD",
        "ARC_DISCARD_EXPLICIT",
        "ARC_DISCARD_DEFAULT",
        "ARC_DISCARD_TAG",
        "ARC_DISCARD_FUNCTOR",
        "ARC_DISCARD_KEYED",
        "ARC_DISCARD_DUCK",
        "ARC_DISCARD_FANOUT",
        "ARC_DISCARD_REPEATER",
        "ARC_DISCARD_SELF",
    ],
    expect = "ignoring return value|call answers nothing here",
    copts = ["-Werror=unused-result"],
)

# A trait reaching a node through a trunk and through another link fails the compile
compile_fail_test(
    name = "trunk_overlaps",
    src = "test_trunk.cpp",
    deps = [
        "//doctest:doctest",
        ":arc_tests_headers_lib",
    ],
    cases = [
        "ARC_TRUNK_OVERLAP_EXACT",
        "ARC_TRUNK_OVERLAP_TRUNKS",
        "ARC_TRUNK_OVERLAP_JOINED",
        "ARC_TRUNK_OVERLAP_FANOUT",
        "ARC_TRUNK_OVERLAP_SINK",
        "ARC_TRUNK_OVERLAP_PARENT",
    ],
    expect = "a trunk cannot be disambiguated",
    # The overlap leaves the trait unresolved, which goes unreported as the node's dependencies go unchecked
    unexpect = "are satisfied by its cluster",
)

# A node taking a cluster's trunk in full without using all of it fails the compile
compile_fail_test(
    name = "trunk_partial_use",
    src = "test_trunk.cpp",
    deps = [
        "//doctest:doctest",
        ":arc_tests_headers_lib",
    ],
    cases = [
        "ARC_TRUNK_PARTIAL_NODE",
        "ARC_TRUNK_PARTIAL_SUBCLUSTER",
        "ARC_TRUNK_PARTIAL_INLINE_SUBCLUSTER",
        "ARC_TRUNK_PARTIAL_PER_TRAIT_SUBCLUSTER",
        "ARC_TRUNK_PARTIAL_SELF_FORWARD_SUBCLUSTER",
    ],
    expect = "without using all of its traits",
)

# A node taking a cluster's trunk in full without listing its dependencies fails the compile
compile_fail_test(
    name = "trunk_undeclared_use",
    src = "test_trunk.cpp",
    deps = [
        "//doctest:doctest",
        ":arc_tests_headers_lib",
    ],
    cases = ["ARC_TRUNK_UNDECLARED"],
    expect = "lists no dependencies to check its use against",
)

###########################################################################
# ABC Module-based library
###########################################################################

arc_module(
    name = "abc_lib",
    arcs = glob(["abc/modules/abc/**/*.ixx.arc"]),
    mods = glob(["abc/modules/abc/*.ixx"]),
    srcs = glob(["abc/modules/abc/*.cpp"]),
    testonly = True,
)

arc_graph(
    name = "abc_alice_graph",
    graph_module = "abc.graph",
    graph_type = "arc::Graph<abc::cluster::AliceBob>",
    nodes = {"alice": "abc.alice"},
    testonly = True,
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
        "test_trait_requires/literals.ixx.arc",
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
        "test_invoke_method.cpp",
        "test_joined_trait.cpp",
        "test_node_count.cpp",
        "test_virtual.cpp",
        "test_contracts.cpp",
        "test_const_trait.cpp",
        "test_default_impl.cpp",
        "test_datastore/test_datastore.cpp",
        "test_datastore/test_layout.cpp",
        "test_datastore/test_lifetime.cpp",
        "test_datastore/test_refs.cpp",
        "test_datastore/test_single_listener.cpp",
        "test_datastore/test_topology.cpp",
        "test_protocol.cpp",
        "test_tracer_spy.cpp",
        "test_trait_requires.cpp",
    ],
    deps = ["//doctest:doctest"],
    testonly = True,
)

arc_module(
    name = "mock_mod",
    embed_arcs = ["test_mock.cpp"],
    deps = ["//doctest:doctest"],
    testonly = True,
)

###########################################################################
# arc_tests_module test binary
###########################################################################

cpp_module_test(
    name = "arc_tests_module",
    srcs = ["test_repeater/main.cpp"] + glob(
        ["test_*.cpp", "test_datastore/*.cpp"],
        exclude = [
            "test_main.cpp",         # test framework entry point, not a test
            "test_abc_headers.cpp",  # arc_tests_headers
            "test_embedded_headers.cpp",
            "test_mem_ptr.cpp",
            "test_nodiscard.cpp",
            "test_trunk.cpp",
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
    deps = ["//doctest:doctest"],
)

###########################################################################
# arc_tests_mock test binary
###########################################################################

cpp_module_test(
    name = "arc_tests_mock",
    srcs = [
        "test_combine.cpp",
        "test_lazy.cpp",
        "test_mock.cpp",
    ],
    module_deps = [":mock_mod"],
    deps = ["//doctest:doctest"],
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
    deps = ["//doctest:doctest"],
    testonly = True,
)

cpp_module_test(
    name = "arc_tests_thread",
    srcs = glob(["test_thread/test_*.cpp"]),
    module_deps = [":thread_lib"],
    deps = ["//doctest:doctest"],
)
