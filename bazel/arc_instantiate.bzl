"""Macros to generate ARC instantiation sources."""

load("@rules_cc//cc:defs.bzl", "cc_library")
load("//bazel:cpp_module.bzl", "cpp_module", "cpp_module_group")
load("@arc_module_deps//:module_deps.bzl", "MODULE_PROVIDER_TARGETS")

def _write_src_impl(ctx):
    out = ctx.actions.declare_file(ctx.attr.out)
    ctx.actions.write(out, ctx.attr.content)
    return [DefaultInfo(files = depset([out]))]

_write_src = rule(
    implementation = _write_src_impl,
    attrs = {
        "out":     attr.string(mandatory = True),
        "content": attr.string(mandatory = True),
    },
)


def _graph_type_hash(graph_type):
    """Compute a stable 7-digit hash string from graph_type.

    Uses Starlark's built-in hash() (Java String.hashCode, guaranteed stable
    within a Bazel version) so that callers don't need to supply a hash.
    """
    h = hash(graph_type)
    if h < 0:
        h = -h
    s = str(h % 10000000)
    return "0" * (7 - len(s)) + s  # zero-pad to 7 digits


def arc_graph(
        name,
        graph_type,
        nodes,
        graph_module = None,
        graph_header = None,
        impl_partition = "impl",
        deps = [],
        visibility = None):
    """Generate and compile ARC instantiation sources.

    Dispatches to a module-based or header-based implementation depending on
    whether graph_module or graph_header is supplied.  Exactly one must be set.

    Module variant (graph_module):
        Creates one cpp_module partition per node, then groups them under
        `name` via cpp_module_group.

        nodes: dict of {node_name: node_module}
            node_name   — dot-path of the node in the graph (e.g. "alice")
            node_module — module that declares the node (e.g. "abc.alice")
        impl_partition: name of the impl partition (default "impl")

    Header variant (graph_header):
        Creates a genrule + cc_library for each node.

        nodes: dict of {node_name: tpp_file}
            node_name — dot-path used in ARC_INSTANTIATE (e.g. "charlie.charlie")
            tpp_file  — include path for the template parameter file
        deps:       CcInfo targets providing headers and include paths
        visibility: Bazel visibility
    """
    if (graph_module == None) == (graph_header == None):
        fail("arc_graph: exactly one of graph_module or graph_header must be set")

    if graph_module != None:
        _arc_instantiate_module(name, graph_module, graph_type, nodes, impl_partition)
    else:
        _arc_instantiate_header(name, graph_header, graph_type, nodes, deps, visibility)


def _arc_instantiate_module(name, graph_module, graph_type, nodes, impl_partition):
    hash_str = _graph_type_hash(graph_type)
    child_targets = []

    for node_name, node_module in nodes.items():
        safe_node = node_module.replace(".", "_").replace(":", "__")
        child_name = name + "__" + safe_node

        out_file = child_name + "_arc_inst.cpp"
        module_name = "{node_module}:{impl_partition}_{hash}".format(
            node_module = node_module,
            impl_partition = impl_partition,
            hash = hash_str,
        )

        src_name = child_name + "_gensrc"
        _write_src(
            name = src_name,
            out = out_file,
            content = """
module;
#include "arc/macros.hpp"
#if !ARC_IMPORT_STD
#include <type_traits>
#endif
module {module_name};
import :{impl_partition};

import {graph_module};
import arc;

ARC_INSTANTIATE(({graph_type}), {node_name})
""".format(
                module_name = module_name,
                impl_partition = impl_partition,
                graph_module = graph_module,
                graph_type = graph_type,
                node_name = node_name,
            ),
        )

        # Direct imports are statically known; resolve provider targets automatically.
        known_direct_deps = [
            "{node_module}:{impl_partition}".format(
                node_module = node_module,
                impl_partition = impl_partition,
            ),
            graph_module,
            "arc",
        ]
        computed_module_deps = [
            MODULE_PROVIDER_TARGETS[m]
            for m in known_direct_deps
            if m in MODULE_PROVIDER_TARGETS
        ]

        cpp_module(
            name = child_name,
            module_name = module_name,
            src = ":" + src_name,
            module_deps = computed_module_deps,
            known_direct_deps = known_direct_deps,
            leaf = True,
        )
        child_targets.append(":" + child_name)

    cpp_module_group(
        name = name,
        module_deps = child_targets,
    )


def _arc_instantiate_header(name, graph_header, graph_type, nodes, deps, visibility):
    gen_srcs = []
    for node_name, tpp_file in nodes.items():
        safe = node_name.replace(".", "_").replace(":", "_")
        gen_name = name + "_gen_" + safe
        out_file = name + "_gensrc/" + node_name + ".cpp"

        _write_src(
            name = gen_name,
            out = out_file,
            content = """
#include "{graph_header}"
#include "arc/graph.hpp"
#include "arc/macros.hpp"
#include "{tpp_file}"
ARC_INSTANTIATE(({graph_type}), {node_name})
""".format(
                graph_header = graph_header,
                tpp_file     = tpp_file,
                graph_type   = graph_type,
                node_name    = node_name,
            ),
        )
        gen_srcs.append(":" + gen_name)

    cc_library(
        name       = name,
        srcs       = gen_srcs,
        deps       = deps,
        visibility = visibility,
    )
