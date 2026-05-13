"""ARC code generation macros for Bazel.

Provides macros for generating C++ modules from ARC DSL files (.ixx.arc, .hxx.arc)
and from embedded arc blocks in .cpp files.

Module names and dependency labels are automatically inferred from MODULE_SOURCES
and MODULE_PROVIDER_TARGETS (pre-scanned at workspace init).  No module_name or
module_deps parameters are required.
"""

load("//bazel:cpp_module.bzl", "cpp_module")
load("@arc_module_deps//:module_deps.bzl",
     "MODULE_DIRECT_DEPS", "MODULE_PROVIDER_TARGETS", "MODULE_SOURCES")
load("@rules_cc//cc:defs.bzl", "cc_library")

_GENERATOR = "//generator_lib:generator"


def _sanitize(s):
    """Convert module name to a valid Bazel target name component."""
    return s.replace(".", "_").replace(":", "__")


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

    Handles four kinds of source files under a single cpp_module target:
      arcs       — .ixx.arc DSL files: run through the ARC generator then compiled
                   as module interfaces.
      mods       — hand-written .ixx module interface files: compiled directly.
      embed_arcs — .cpp files containing arc-begin/arc-end blocks: the arc block is
                   extracted by the generator and compiled as a module interface.
      srcs       — .cpp implementation files: compiled against the modules above.

    Each file still gets its own compile action (cpp_module loops internally), but
    everything is bundled into a single cpp_module target that exposes both
    CppModuleInfo (for module_deps) and CcInfo (for deps / linking).

    Args:
        name:        Target name for the resulting cpp_module.
        srcs:        List of .cpp implementation files.
        mods:        List of hand-written .ixx module interface files.
        arcs:        List of .ixx.arc ARC DSL source files to generate from.
        embed_arcs:  List of .cpp files containing embedded arc-begin/arc-end blocks.
        module_deps: Optional CppModuleInfo targets not covered by the scanner.
        deps:        List of CcInfo targets (headers etc).
        includes:    Additional include directories.
        defines:     Additional preprocessor defines.
        copts:       Additional compiler options.
        **kwargs:    Forwarded to the cpp_module target (e.g. tags, testonly, visibility).
    """
    tags = kwargs.get("tags", [])
    testonly = kwargs.get("testonly", False)
    pkg = native.package_name()

    all_mods = []
    module_names = {}
    computed_module_deps = list(module_deps)
    provider_aliases = []  # (alias_name, module_name) — emitted after cpp_module
    provided_modules = {}  # module names provided by this target (skip self-deps)

    def _lookup_module_name(src, kind):
        key = pkg + "/" + src
        module_name = MODULE_SOURCES.get(key)
        if module_name == None:
            fail(
                "arc_module_library: cannot find module name for '{}' ({}). ".format(key, kind) +
                "Key not found in MODULE_SOURCES. Run bazel clean --expunge to refresh the scan."
            )
        return module_name

    # --- .ixx.arc files: run generator then compile ---
    for src in arcs:
        module_name = _lookup_module_name(src, "arcs")
        sanitized       = _sanitize(module_name)
        gen_target_name = "_gen_arcgen_" + sanitized
        out_ixx         = gen_target_name + ".ixx"
        native.genrule(
            name = gen_target_name,
            srcs = [src],
            outs = [out_ixx],
            cmd = "$(execpath {gen}) -q -m -i $(location {src}) -o $@".format(
                gen = _GENERATOR, src = src),
            tools = [_GENERATOR],
            visibility = ["//visibility:private"],
            tags = tags,
            testonly = testonly,
        )
        all_mods.append(":" + gen_target_name)
        module_names[pkg + "/" + out_ixx] = module_name
        provider_aliases.append(("_arcgen_" + sanitized, module_name))
        provided_modules[module_name] = True

    # --- .ixx files: scanner already knows them ---
    for src in mods:
        module_name = _lookup_module_name(src, "mods")
        all_mods.append(src)
        provider_aliases.append(("_ixx_" + _sanitize(module_name), module_name))
        provided_modules[module_name] = True

    # --- .cpp files with embedded arc blocks: extract then compile ---
    for src in embed_arcs:
        module_name = _lookup_module_name(src, "embed_arcs")
        sanitized      = _sanitize(module_name)
        embed_gen_name = "_embed_arcembed_" + sanitized
        out_ixx        = embed_gen_name + ".ixx"
        native.genrule(
            name = embed_gen_name,
            srcs = [src],
            outs = [out_ixx],
            cmd = "$(execpath {gen}) -q -m -i $(location {src}) -o $@".format(
                gen = _GENERATOR, src = src),
            tools = [_GENERATOR],
            visibility = ["//visibility:private"],
            tags = tags,
            testonly = testonly,
        )
        all_mods.append(":" + embed_gen_name)
        module_names[pkg + "/" + out_ixx] = module_name
        provider_aliases.append(("_arcembed_" + sanitized, module_name))
        provided_modules[module_name] = True

    # Resolve external module_deps: skip any dep already provided by this target.
    for module_name in provided_modules:
        for dep_name in MODULE_DIRECT_DEPS.get(module_name, []):
            if dep_name in provided_modules:
                continue
            label = MODULE_PROVIDER_TARGETS.get(dep_name)
            if label != None and label not in computed_module_deps:
                computed_module_deps.append(label)

    # Arc-generated modules need arc_headers (generator emits the include).
    arc_headers = "//lib:arc_headers"
    final_deps = list(deps)
    if (arcs or embed_arcs) and arc_headers not in final_deps:
        final_deps = [arc_headers] + final_deps

    cpp_module(
        name = name,
        mods = all_mods,
        srcs = srcs,
        module_names = module_names,
        module_deps = computed_module_deps,
        deps = final_deps,
        includes = includes,
        defines = defines,
        copts = copts,
        **kwargs
    )

    # Aliases so that consumers wired by the scanner's MODULE_PROVIDER_TARGETS
    # naming convention (_ixx_*, _arcgen_*, _arcembed_*) resolve to the
    # bundled cpp_module target.
    visibility = kwargs.get("visibility", ["//visibility:public"])
    for alias_name, _module_name in provider_aliases:
        native.alias(
            name = alias_name,
            actual = ":" + name,
            visibility = visibility,
            tags = tags,
            testonly = testonly,
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
