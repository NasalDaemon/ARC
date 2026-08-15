"""Macros to generate ARC instantiation sources."""

load("@rules_cc//cc:defs.bzl", "cc_library")
load("//bazel:cpp_module.bzl", "cpp_module")
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


_IDENT_CHARS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_"


def _safe_ident(node_path):
    """Turn a node path into a valid C++ identifier / Bazel target fragment.

    Node paths carry arbitrary C++ punctuation -- `main.channels->atId(
    pax::ChannelId{}).dag` -- none of which is legal in a module-partition
    name or a target name.  Every character outside [A-Za-z0-9_] is mapped
    to a single `_`, one-for-one (no run collapsing), so the mapping stays
    injective and two distinct node paths can never sanitise to the same
    identifier.
    """
    out = []
    for ch in node_path.elems():
        out.append(ch if ch in _IDENT_CHARS else "_")
    s = "".join(out)

    # A partition name must not start with a digit.
    if s and s[0] in "0123456789":
        s = "_" + s
    return s


def arc_graph(
        name,
        graph_type,
        nodes,
        graph_module = None,
        graph_header = None,
        impl_partition = "impl",
        common_modules = [],
        deps = [],
        **kwargs):
    """Generate and compile ARC instantiation sources.

    Dispatches to a module-based or header-based implementation depending on
    whether graph_module or graph_header is supplied.  Exactly one must be set.

    Module variant (graph_module):
        Bundles one module partition per node into a single cpp_module target
        named `name`.

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

    **kwargs: Forwarded to the outer target (e.g. tags, testonly, visibility).
    """
    if (graph_module == None) == (graph_header == None):
        fail("arc_graph: exactly one of graph_module or graph_header must be set")

    if graph_module != None:
        _arc_instantiate_module(name, graph_module, graph_type, nodes, impl_partition, common_modules, kwargs)
    else:
        _arc_instantiate_header(name, graph_header, graph_type, nodes, deps, kwargs)


def _arc_instantiate_module(name, graph_module, graph_type, nodes, impl_partition, common_modules, kwargs):
    hash_str = _graph_type_hash(graph_type)
    tags = kwargs.get("tags", [])
    testonly = kwargs.get("testonly", False)
    pkg = native.package_name()

    all_mods = []
    module_names = {}
    known_direct_deps_map = {}
    computed_module_deps = []
    seen = {}

    common_imports = "\n".join(["import {m};".format(m = m) for m in common_modules])

    for node_name, node_module in nodes.items():
        # Key names on the NODE PATH, not the module: a graph may hold several
        # nodes of the same module (e.g. two arc::DynamicMap collections whose
        # elements are the same cluster type), and keying on the module makes
        # those collide on both the generated target name and the module
        # partition name.  Node paths are unique within a graph by construction.
        safe_node = _safe_ident(node_name)
        child_prefix = name + "__" + safe_node
        out_file = child_prefix + "_arc_inst.ixx"
        src_name = child_prefix + "_gensrc"

        module_name = "{node_module}:{impl_partition}_{safe_node}_{hash}".format(
            node_module = node_module,
            impl_partition = impl_partition,
            safe_node = safe_node,
            hash = hash_str,
        )

        _write_src(
            name = src_name,
            out = out_file,
            tags = tags,
            testonly = testonly,
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
{common_imports}

ARC_INSTANTIATE(({graph_type}), {node_name})
""".format(
                module_name = module_name,
                impl_partition = impl_partition,
                graph_module = graph_module,
                graph_type = graph_type,
                node_name = node_name,
                common_imports = common_imports,
            ),
        )

        all_mods.append(":" + src_name)
        module_names[pkg + "/" + out_file] = module_name

        direct_deps = [
            "{node_module}:{impl_partition}".format(
                node_module = node_module, impl_partition = impl_partition),
            graph_module,
            "arc",
        ] + list(common_modules)
        known_direct_deps_map[module_name] = direct_deps
        for dep_module in direct_deps:
            label = MODULE_PROVIDER_TARGETS.get(dep_module)
            if label != None and label not in seen:
                seen[label] = True
                computed_module_deps.append(label)

    cpp_module(
        name = name,
        mods = all_mods,
        module_names = module_names,
        module_deps = computed_module_deps,
        known_direct_deps_map = known_direct_deps_map,
        leaf = True,
        **kwargs
    )


def _arc_instantiate_header(name, graph_header, graph_type, nodes, deps, kwargs):
    gen_srcs = []
    tags = kwargs.get("tags", [])
    testonly = kwargs.get("testonly", False)

    for node_name, tpp_file in nodes.items():
        safe = node_name.replace(".", "_").replace(":", "_")
        gen_name = name + "_gen_" + safe
        out_file = name + "_gensrc/" + node_name + ".cpp"

        _write_src(
            name = gen_name,
            out = out_file,
            tags = tags,
            testonly = testonly,
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
        name = name,
        srcs = gen_srcs,
        deps = deps,
        **kwargs
    )
