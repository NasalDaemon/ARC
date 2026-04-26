"""ARC code generation macros for Bazel.

Provides macros for generating C++ modules from ARC DSL files (.ixx.arc, .hxx.arc)
and from embedded arc blocks in .cpp files.

Module names and dependency labels are automatically inferred from MODULE_SOURCES
and MODULE_PROVIDER_TARGETS (pre-scanned at workspace init).  No module_name or
module_deps parameters are required.
"""

load("//bazel:cpp_module.bzl", "cpp_module", "cpp_module_compile_srcs", "cpp_module_group")
load("@arc_module_deps//:module_deps.bzl",
     "MODULE_DIRECT_DEPS", "MODULE_PROVIDER_TARGETS", "MODULE_SOURCES")
load("@rules_cc//cc:defs.bzl", "cc_library")

_GENERATOR = "//generator_lib:generator"


def _sanitize(s):
    """Convert module name to a valid Bazel target name component."""
    return s.replace(".", "_").replace(":", "__")


def _resolve_module_deps(module_name, extra_module_deps):
    """Build the module_deps list for a given module_name.

    Starts with extra_module_deps (explicit overrides), then appends
    targets inferred from MODULE_DIRECT_DEPS + MODULE_PROVIDER_TARGETS.
    """
    computed = list(extra_module_deps)
    for dep_name in MODULE_DIRECT_DEPS.get(module_name, []):
        label = MODULE_PROVIDER_TARGETS.get(dep_name)
        if label != None and label not in computed:
            computed.append(label)
    return computed


def arc_module_library(
        name,
        srcs = [],
        mods = [],
        arcs = [],
        embed_arcs = [],
        module_deps = [],
        deps = [],
        includes = [],
        defines = [],
        copts = [],
        **kwargs):
    """All-in-one rule for ARC module libraries.

    Handles four kinds of source files under a single target:
      arcs       — .ixx.arc DSL files: run through the ARC generator then compiled
                   as module interfaces.
      mods       — hand-written .ixx module interface files: compiled directly.
      embed_arcs — .cpp files containing arc-begin/arc-end blocks: the arc block is
                   extracted by the generator and compiled as a module interface.
      srcs       — .cpp implementation files: compiled against the modules above
                   (equivalent to cpp_module_compile_srcs).

    Each file gets its own compile action for fine-grained dependency tracking.
    The resulting target exposes CppModuleInfo (for module_deps) and CcInfo
    (for deps / linking) through a single cpp_module_group.

    Args:
        name:        Target name for the resulting cpp_module_group.
        srcs:        List of .cpp implementation files.
        mods:        List of hand-written .ixx module interface files.
        arcs:        List of .ixx.arc ARC DSL source files to generate from.
        embed_arcs:  List of .cpp files containing embedded arc-begin/arc-end blocks.
        module_deps: Optional CppModuleInfo targets not covered by the scanner.
        deps:        List of CcInfo targets (headers etc).
        includes:    Additional include directories.
        defines:     Additional preprocessor defines.
        copts:       Additional compiler options.
        **kwargs:    Forwarded to the outer cpp_module_group (e.g. tags, testonly, visibility).
    """

    # Arc-generated modules always need arc_headers (the generator emits the include).
    arc_headers = "//lib:arc_headers"
    arc_deps = [arc_headers] + [d for d in deps if d != arc_headers]

    tags = kwargs.get("tags", [])
    testonly = kwargs.get("testonly", False)

    all_module_targets = []

    # --- .ixx.arc files: run generator then compile ---
    for src in arcs:
        key = native.package_name() + "/" + src
        module_name = MODULE_SOURCES.get(key)
        if module_name == None:
            fail(
                "arc_module_library: cannot find module name for '{}'. ".format(key) +
                "Key not found in MODULE_SOURCES. " +
                "Make sure the .ixx.arc file produces 'export module <name>;' and that " +
                "bazel clean --expunge has been run to refresh the module scan."
            )

        sanitized       = _sanitize(module_name)
        gen_target_name = "_gen_arcgen_" + sanitized
        arcgen_name     = "_arcgen_" + sanitized

        native.genrule(
            name = gen_target_name,
            srcs = [src],
            outs = [gen_target_name + ".ixx"],
            cmd = "$(execpath {gen}) -q -m -i $(location {src}) -o $@".format(
                gen = _GENERATOR,
                src = src,
            ),
            tools = [_GENERATOR],
            visibility = ["//visibility:private"],
            tags = tags,
            testonly = testonly,
        )

        computed_deps = _resolve_module_deps(module_name, module_deps)

        cpp_module(
            name = arcgen_name,
            module_name = module_name,
            src = ":" + gen_target_name,
            module_deps = computed_deps,
            deps = arc_deps,
            defines = defines,
            copts = copts,
            visibility = ["//visibility:private"],
            tags = tags,
            testonly = testonly,
        )

        all_module_targets.append(":" + arcgen_name)

    # --- .ixx files: compile directly ---
    for src in mods:
        key = native.package_name() + "/" + src
        module_name = MODULE_SOURCES.get(key)
        if module_name == None:
            fail(
                "arc_module_library: cannot find module name for '{}'. ".format(key) +
                "Key not found in MODULE_SOURCES. " +
                "Make sure the file contains 'export module <name>;' and that " +
                "bazel clean --expunge has been run to refresh the module scan."
            )

        target_name = "_ixx_" + _sanitize(module_name)

        computed_deps = list(module_deps)
        for dep_name in MODULE_DIRECT_DEPS.get(module_name, []):
            label = MODULE_PROVIDER_TARGETS.get(dep_name)
            if label != None and label not in computed_deps:
                computed_deps.append(label)

        cpp_module(
            name = target_name,
            module_name = module_name,
            src = src,
            module_deps = computed_deps,
            deps = deps,
            includes = includes,
            defines = defines,
            copts = copts,
            visibility = ["//visibility:private"],
            tags = tags,
            testonly = testonly,
        )

        all_module_targets.append(":" + target_name)

    # --- .cpp files with embedded arc blocks: extract module interface then compile ---
    for src in embed_arcs:
        key = native.package_name() + "/" + src
        module_name = MODULE_SOURCES.get(key)
        if module_name == None:
            fail(
                "arc_module_library: cannot find module name for '{}'. ".format(key) +
                "Key not found in MODULE_SOURCES. " +
                "Make sure the .cpp file contains an arc-begin/arc-end block with " +
                "'export module <name>;' and that bazel clean --expunge has been run."
            )

        sanitized        = _sanitize(module_name)
        embed_gen_name   = "_embed_arcembed_" + sanitized
        arcembed_name    = "_arcembed_" + sanitized

        native.genrule(
            name = embed_gen_name,
            srcs = [src],
            outs = [embed_gen_name + ".ixx"],
            cmd = "$(execpath {gen}) -q -m -i $(location {src}) -o $@".format(
                gen = _GENERATOR,
                src = src,
            ),
            tools = [_GENERATOR],
            visibility = ["//visibility:private"],
            tags = tags,
            testonly = testonly,
        )

        computed_deps = _resolve_module_deps(module_name, module_deps)

        cpp_module(
            name = arcembed_name,
            module_name = module_name,
            src = ":" + embed_gen_name,
            module_deps = computed_deps,
            deps = arc_deps,
            includes = includes,
            defines = defines,
            copts = copts,
            visibility = ["//visibility:private"],
            tags = tags,
            testonly = testonly,
        )

        all_module_targets.append(":" + arcembed_name)

    # --- .cpp files: compile against the modules above ---
    if srcs:
        impl_name = name + "_impl"
        cpp_module_compile_srcs(
            name = impl_name,
            srcs = srcs,
            module_deps = all_module_targets,
            deps = deps,
            includes = includes,
            defines = defines,
            copts = copts,
            **kwargs
        )
        group_deps = [":" + impl_name]
    else:
        group_deps = deps

    cpp_module_group(
        name = name,
        module_deps = all_module_targets,
        deps = group_deps,
        **kwargs
    )


def arc_header_library(
        name,
        arcs = [],
        hdrs = [],
        srcs = [],
        embed_arcs = [],
        deps = [],
        includes = [],
        defines = [],
        copts = [],
        **kwargs):
    """All-in-one rule for ARC header-based libraries.

    Handles four kinds of source files under a single cc_library target:
      arcs       — .hxx.arc DSL files: run through the ARC generator to produce .hxx headers.
      hdrs       — hand-written header files (.hpp, .hxx, .tpp, etc.).
      embed_arcs — dict of {src: include_path}: .cpp files containing arc-begin/arc-end
                   blocks, where include_path is the header path consumers will #include.
                   A synthetic include directory is created automatically; no includes= entry
                   is needed for these.
      srcs       — .cpp implementation files compiled into the library.

    Args:
        name:        Target name for the resulting cc_library.
        arcs:        List of .hxx.arc ARC DSL source files; output path inferred by
                     stripping the .arc suffix.
        hdrs:        List of hand-written header files to expose.
        srcs:        List of .cpp implementation files.
        embed_arcs:  Dict of {src: include_path} — .cpp files containing embedded arc
                     blocks; include_path is the path consumers use in #include <...>.
        deps:        List of CcInfo targets (headers, libraries).
        includes:    Include directories to expose to consumers (for arcs/hdrs).
        defines:     Preprocessor defines.
        copts:       Additional compiler options (for srcs).
        **kwargs:    Forwarded to the outer cc_library (e.g. tags, testonly, visibility).
    """
    tags = kwargs.get("tags", [])
    testonly = kwargs.get("testonly", False)
    gen_hdrs = []
    all_includes = list(includes)

    for src in arcs:
        if not src.endswith(".arc"):
            fail("arc_header_library: arcs entry '{}' must end with '.arc'".format(src))
        out = src[:-4]
        gen_name = "_arc_hdr_" + src.replace("/", "_").replace(".", "_")
        native.genrule(
            name = gen_name,
            srcs = [src],
            outs = [out],
            cmd = "$(execpath {gen}) -q -i $(location {src}) -o $@".format(
                gen = _GENERATOR,
                src = src,
            ),
            tools = [_GENERATOR],
            visibility = ["//visibility:private"],
            tags = tags,
            testonly = testonly,
        )
        gen_hdrs.append(":" + gen_name)

    if embed_arcs:
        synth_dir = "_arc_embed_hdrs_" + name
        all_includes.append(synth_dir)
        for src, include_path in embed_arcs.items():
            gen_name = "_arc_hdr_" + src.replace("/", "_").replace(".", "_")
            native.genrule(
                name = gen_name,
                srcs = [src],
                outs = [synth_dir + "/" + include_path],
                cmd = "$(execpath {gen}) -q -i $(location {src}) -o $@".format(
                    gen = _GENERATOR,
                    src = src,
                ),
                tools = [_GENERATOR],
                visibility = ["//visibility:private"],
                tags = tags,
                testonly = testonly,
            )
            gen_hdrs.append(":" + gen_name)

    cc_library(
        name = name,
        hdrs = gen_hdrs + hdrs,
        srcs = srcs,
        includes = all_includes,
        deps = deps,
        defines = defines,
        copts = copts,
        **kwargs
    )


def arc_gen_headers(name, srcs, outs = None, includes = [], **kwargs):
    """Generate header files from .arc files and expose as a cc_library.

    Output paths are inferred by stripping the .arc suffix from each src,
    preserving directory structure.  Pass explicit outs= for non-.arc sources
    (e.g. .cpp files containing embedded arc blocks).

    Args:
        name:     Target name for the resulting cc_library.
        srcs:     List of source files (.arc or other if outs is provided).
        outs:     Optional list of explicit output paths, one per src.
                  Required when srcs contain non-.arc files.
        includes: Include directories to expose via the cc_library.
        **kwargs: Forwarded to the outer cc_library (e.g. tags, testonly, visibility).
    """
    tags = kwargs.get("tags", [])
    testonly = kwargs.get("testonly", False)
    gen_targets = []

    for i, src in enumerate(srcs):
        if outs != None:
            out = outs[i]
        elif src.endswith(".arc"):
            out = src[:-4]  # strip .arc suffix
        else:
            fail("arc_gen_headers: src '{}' does not end with '.arc'; provide explicit outs=".format(src))

        gen_name = "_arc_hdr_" + src.replace("/", "_").replace(".", "_")

        native.genrule(
            name = gen_name,
            srcs = [src],
            outs = [out],
            cmd = "$(execpath {gen}) -q -i $(location {src}) -o $@".format(
                gen = _GENERATOR,
                src = src,
            ),
            tools = [_GENERATOR],
            visibility = ["//visibility:private"],
            tags = tags,
            testonly = testonly,
        )

        gen_targets.append(":" + gen_name)

    cc_library(
        name = name,
        hdrs = gen_targets,
        includes = includes,
        **kwargs
    )
