"""Custom rules for C++ module compilation (Clang and GCC)."""

load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")
load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc:action_names.bzl", "CPP_LINK_STATIC_LIBRARY_ACTION_NAME")
load("@rules_cc//cc:find_cc_toolchain.bzl", "CC_TOOLCHAIN_ATTRS", "use_cc_toolchain")
load("@arc_module_deps//:module_deps.bzl", "FILE_DIRECT_DEPS", "MODULE_DIRECT_DEPS", "MODULE_SOURCES")

CppModuleInfo = provider(
    doc = "Information about compiled C++ modules",
    fields = {
        "module_name": "string: full module name (e.g., 'arc', 'abc.alice', 'abc.alice:impl')",
        "pcm": "File: precompiled module (.pcm)",
        "obj": "File: object file from PCM",
        "transitive_modules": "list of struct(name, pcm, obj, direct_deps): all transitive modules. " +
                              "direct_deps is a list of module name strings, or None if unknown.",
        "transitive_hdrs": "depset of File: all transitive header files",
        "transitive_includes": "list of string: include directories",
        "transitive_defines": "list of string: defines",
        "transitive_link_cc_infos": "CcInfo or None: merged link-time CcInfo from implementation deps " +
                                    "(cpp_module_compile_srcs outputs), propagated through module_deps chains " +
                                    "so outer cpp_module_group targets inherit it without explicit deps=.",
    },
)

# GCC embeds a timestamp in .gcm files; SOURCE_DATE_EPOCH=0 makes output deterministic.
_GCC_ENV = {"SOURCE_DATE_EPOCH": "0"}

# ---------------------------------------------------------------------------
# Dependency computation helpers
# ---------------------------------------------------------------------------

def _compute_needed_modules(all_modules, direct_imports):
    """BFS over the module dep graph to find the minimal reachable set.

    Starlark has no while loop, so we use BFS level-by-level with a for loop
    bounded by len(all_modules) — sufficient for any finite DAG.

    Args:
        all_modules: list of struct(name, pcm, obj, direct_deps)
        direct_imports: list of module name strings to start BFS from,
                        or None to indicate 'unknown' (returns all_modules).

    Returns: list of module structs transitively reachable from direct_imports.
    """
    if direct_imports == None:
        return all_modules

    by_name = {}
    for m in all_modules:
        by_name[m.name] = m

    needed   = {}
    frontier = list(direct_imports)

    # Each pass expands one level of the dep graph.
    # At most len(all_modules) passes are needed for any finite DAG.
    for _pass in range(len(all_modules) + 1):
        if not frontier:
            break
        next_frontier = []
        for name in frontier:
            if name in needed or name not in by_name:
                continue
            m = by_name[name]
            needed[name] = m

            # direct_deps may be None for modules that couldn't be scanned;
            # fall back conservatively to the full set.
            m_deps = getattr(m, "direct_deps", None)
            if m_deps == None:
                return list(all_modules)
            for dep in m_deps:
                if dep not in needed:
                    next_frontier.append(dep)
        frontier = next_frontier

    return list(needed.values())

def _module_flags(modules):
    """Return (-fmodule-file= flag strings, pcm file list) for a module list (Clang)."""
    flags = []
    pcms  = []
    for m in modules:
        flags.append("-fmodule-file={}={}".format(m.name, m.pcm.path))
        pcms.append(m.pcm)
    return flags, pcms

def _gcc_write_mapper(ctx, module_name, gcm_output, dep_modules, suffix = ""):
    """Write a GCC module mapper file; returns (mapper_file, [gcm files for inputs]).

    Maps the current module (if any) and all dep modules to their .gcm paths so
    GCC can find them via -fmodule-mapper=.
    """
    lines = ["MAPPING"]
    if module_name and gcm_output:
        lines.append("{} {}".format(module_name, gcm_output.path))
    for m in dep_modules:
        if m.pcm != None:
            lines.append("{} {}".format(m.name, m.pcm.path))
    mapper = ctx.actions.declare_file(ctx.label.name + suffix + ".modmap")
    ctx.actions.write(mapper, "\n".join(lines) + "\n")
    dep_gcms = [m.pcm for m in dep_modules if m.pcm != None]
    return mapper, dep_gcms

# ---------------------------------------------------------------------------
# Toolchain helpers
# ---------------------------------------------------------------------------

def _get_compiler_and_toolchain(ctx):
    cc_toolchain = find_cpp_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )
    compiler = cc_common.get_tool_for_action(
        feature_configuration = feature_configuration,
        action_name = ACTION_NAMES.cpp_compile,
    )
    is_gcc = "gcc" in cc_toolchain.compiler and "clang" not in cc_toolchain.compiler
    return compiler, cc_toolchain, feature_configuration, is_gcc

def _stdlib_flags(ctx, is_gcc):
    """Returns any -stdlib= flag the user configured, or [] for GCC (which ignores it)."""
    if is_gcc:
        return []
    return [opt for opt in ctx.fragments.cpp.cxxopts if opt.startswith("-stdlib=")]

# ---------------------------------------------------------------------------
# Provider collection helpers
# ---------------------------------------------------------------------------

def _collect_module_info(deps):
    """Collect transitive module information from CppModuleInfo deps."""
    all_modules = []
    all_hdrs    = []
    all_includes = []
    all_defines  = []
    seen = {}

    for dep in deps:
        if CppModuleInfo in dep:
            mi = dep[CppModuleInfo]
            for m in mi.transitive_modules:
                if m.name not in seen:
                    seen[m.name] = True
                    all_modules.append(m)
            all_hdrs.append(mi.transitive_hdrs)
            for inc in mi.transitive_includes:
                if inc not in all_includes:
                    all_includes.append(inc)
            for d in mi.transitive_defines:
                if d not in all_defines:
                    all_defines.append(d)

    return all_modules, all_hdrs, all_includes, all_defines

def _collect_cc_info(deps):
    """Collect headers and includes from CcInfo deps."""
    all_hdrs    = []
    all_includes = []
    for dep in deps:
        if CcInfo in dep:
            ctx = dep[CcInfo].compilation_context
            all_hdrs.append(ctx.headers)
            for inc in ctx.includes.to_list():
                if inc not in all_includes:
                    all_includes.append(inc)
            for inc in ctx.system_includes.to_list():
                if inc not in all_includes:
                    all_includes.append(inc)
    return all_hdrs, all_includes

# ---------------------------------------------------------------------------
# cpp_module rule
# ---------------------------------------------------------------------------

def _cpp_module_impl(ctx):
    compiler, cc_toolchain, feature_configuration, is_gcc = _get_compiler_and_toolchain(ctx)

    mod_modules, mod_hdrs, mod_includes, mod_defines = _collect_module_info(ctx.attr.module_deps)
    cc_hdrs, cc_includes = _collect_cc_info(ctx.attr.deps)

    all_includes = mod_includes + ctx.attr.includes
    for inc in cc_includes:
        if inc not in all_includes:
            all_includes.append(inc)
    all_defines = mod_defines + ctx.attr.defines
    src         = ctx.file.src

    # Determine direct deps for this module:
    #   1. Explicitly provided via known_direct_deps (arc_instantiate_module).
    #   2. Looked up from the pre-scanned dep map (MODULE_DIRECT_DEPS).
    #      The scanner runs the ARC generator on .ixx.arc and arc-embed files
    #      during workspace init, so MODULE_DIRECT_DEPS is accurate for both
    #      hand-written and ARC-generated module sources.
    #   3. None → module not found in dep map, safe fallback: all PCMs.
    if ctx.attr.known_direct_deps:
        direct_deps_list = ctx.attr.known_direct_deps
    else:
        direct_deps_list = MODULE_DIRECT_DEPS.get(ctx.attr.module_name)

    needed_modules = _compute_needed_modules(mod_modules, direct_deps_list)
    module_flags, module_pcm_files = _module_flags(needed_modules)

    include_flags = ["-I" + inc for inc in all_includes]
    define_flags  = ["-D" + d   for d   in all_defines]
    all_hdrs      = mod_hdrs + cc_hdrs

    if is_gcc:
        gcm = ctx.actions.declare_file(ctx.label.name + ".gcm")
        obj = ctx.actions.declare_file(ctx.label.name + ".o")
        mapper, dep_gcms = _gcc_write_mapper(
            ctx, ctx.attr.module_name, gcm, needed_modules)
        if ctx.attr.leaf:
            # Leaf module: nothing imports the .gcm, so combine precompile + compile
            # into one pass (no -fmodule-only). GCC writes both .gcm (via mapper)
            # and .o (via -o) in a single invocation.
            ctx.actions.run(
                inputs = depset(
                    direct    = [src, mapper] + dep_gcms,
                    transitive = all_hdrs + [cc_toolchain.all_files],
                ),
                outputs    = [gcm, obj],
                executable = compiler,
                env        = _GCC_ENV,
                arguments  = [
                    "-std=c++23", "-fmodules-ts",
                    "-fmodule-mapper=" + mapper.path,
                    "-x", "c++", "-c", src.path, "-o", obj.path,
                ] + include_flags + define_flags + ctx.attr.copts,
                mnemonic         = "CppModuleCompile",
                progress_message = "Compiling C++ module {} [{}]".format(
                    ctx.attr.module_name, ctx.label),
            )
        else:
            # Non-leaf: two-step so dependents can start their precompile as soon as
            # this module's .gcm is ready, while this .o compiles in parallel.
            # Step 1: .ixx → .gcm (CMI only)
            ctx.actions.run(
                inputs = depset(
                    direct    = [src, mapper] + dep_gcms,
                    transitive = all_hdrs + [cc_toolchain.all_files],
                ),
                outputs    = [gcm],
                executable = compiler,
                env        = _GCC_ENV,
                arguments  = [
                    "-std=c++23", "-fmodules-ts",
                    "-fmodule-mapper=" + mapper.path,
                    "-fmodule-only", "-x", "c++", "-c", src.path,
                ] + include_flags + define_flags + ctx.attr.copts,
                mnemonic         = "CppModulePrecompile",
                progress_message = "Precompiling C++ module {} [{}]".format(
                    ctx.attr.module_name, ctx.label),
            )
            # Step 2: .ixx → .o  (dep on gcm to serialize after step 1)
            ctx.actions.run(
                inputs = depset(
                    direct    = [src, mapper, gcm] + dep_gcms,
                    transitive = all_hdrs + [cc_toolchain.all_files],
                ),
                outputs    = [obj],
                executable = compiler,
                env        = _GCC_ENV,
                arguments  = [
                    "-std=c++23", "-fmodules-ts",
                    "-fmodule-mapper=" + mapper.path,
                    "-x", "c++", "-c", src.path, "-o", obj.path,
                ] + include_flags + define_flags + ctx.attr.copts,
                mnemonic         = "CppModuleCompileObj",
                progress_message = "Compiling C++ module object {}".format(ctx.label.name),
            )
        pcm = gcm
    else:
        stdlib = _stdlib_flags(ctx, is_gcc)
        # Clang: step 1: .ixx / generated .ixx  →  .pcm
        pcm = ctx.actions.declare_file(ctx.label.name + ".pcm")
        ctx.actions.run(
            inputs = depset(
                direct    = [src] + module_pcm_files,
                transitive = all_hdrs + [cc_toolchain.all_files],
            ),
            outputs    = [pcm],
            executable = compiler,
            arguments  = [
                "-std=c++23",
                "-x", "c++-module", "--precompile",
            ] + stdlib + module_flags + include_flags + define_flags + ctx.attr.copts + [
                src.path, "-o", pcm.path,
            ],
            mnemonic         = "CppModulePrecompile",
            progress_message = "Precompiling C++ module {} [{}]".format(
                ctx.attr.module_name, ctx.label),
        )

        # Clang: step 2: .pcm  →  .o
        obj = ctx.actions.declare_file(ctx.label.name + ".o")
        ctx.actions.run(
            inputs = depset(
                direct    = [pcm] + module_pcm_files,
                transitive = [cc_toolchain.all_files],
            ),
            outputs    = [obj],
            executable = compiler,
            arguments  = [
                "-std=c++23",
                "-c", pcm.path, "-o", obj.path,
            ] + module_flags,
            mnemonic         = "CppModuleCompileObj",
            progress_message = "Compiling C++ module object {}".format(ctx.label.name),
        )

    my_entry = struct(
        name        = ctx.attr.module_name,
        pcm         = pcm,
        obj         = obj,
        direct_deps = direct_deps_list,
    )

    return [
        CppModuleInfo(
            module_name       = ctx.attr.module_name,
            pcm               = pcm,
            obj               = obj,
            transitive_modules = mod_modules + [my_entry],
            transitive_hdrs   = depset(transitive = all_hdrs),
            transitive_includes = all_includes,
            transitive_defines  = all_defines,
            transitive_link_cc_infos = None,
        ),
        DefaultInfo(files = depset([pcm, obj])),
    ]

cpp_module = rule(
    implementation = _cpp_module_impl,
    attrs = {
        "module_name":       attr.string(mandatory = True),
        "src":               attr.label(allow_single_file = True, mandatory = True),
        "module_deps":       attr.label_list(providers = [CppModuleInfo]),
        "deps":              attr.label_list(providers = [CcInfo]),
        "includes":          attr.string_list(),
        "defines":           attr.string_list(),
        "copts":             attr.string_list(),
        # Explicit direct deps for generated sources whose imports are known
        # from rule parameters (e.g. arc_instantiate_module).
        # Overrides the MODULE_DIRECT_DEPS lookup when non-empty.
        "known_direct_deps": attr.string_list(default = []),
        # Leaf modules (nothing imports their .gcm) can skip the two-step GCC
        # precompile/compile split and produce .gcm + .o in a single pass.
        "leaf": attr.bool(default = False),
    } | CC_TOOLCHAIN_ATTRS,
    toolchains = use_cc_toolchain(),
    fragments  = ["cpp"],
)

# ---------------------------------------------------------------------------
# cpp_module_group rule  (archives transitive objects → CcInfo for linking)
# ---------------------------------------------------------------------------

def _create_static_lib(ctx, cc_toolchain, feature_configuration, objs, lib_name):
    output_file = ctx.actions.declare_file(lib_name)

    archiver_path = cc_common.get_tool_for_action(
        feature_configuration = feature_configuration,
        action_name = CPP_LINK_STATIC_LIBRARY_ACTION_NAME,
    )
    archiver_variables = cc_common.create_link_variables(
        feature_configuration = feature_configuration,
        cc_toolchain          = cc_toolchain,
        output_file           = output_file.path,
        is_using_linker       = False,
    )
    command_line = cc_common.get_memory_inefficient_command_line(
        feature_configuration = feature_configuration,
        action_name           = CPP_LINK_STATIC_LIBRARY_ACTION_NAME,
        variables             = archiver_variables,
    )
    env = cc_common.get_environment_variables(
        feature_configuration = feature_configuration,
        action_name           = CPP_LINK_STATIC_LIBRARY_ACTION_NAME,
        variables             = archiver_variables,
    )

    args = ctx.actions.args()
    args.add_all(command_line)
    args.add_all(objs)

    ctx.actions.run(
        executable  = archiver_path,
        arguments   = [args],
        env         = env,
        mnemonic    = "CppModuleArchive",
        inputs      = depset(direct = objs, transitive = [cc_toolchain.all_files]),
        outputs     = [output_file],
        progress_message = "Archiving module objects into {}".format(lib_name),
    )
    return output_file

def _cpp_module_group_impl(ctx):
    mod_modules, mod_hdrs, mod_includes, mod_defines = _collect_module_info(ctx.attr.module_deps)
    cc_hdrs, cc_includes = _collect_cc_info(ctx.attr.deps)

    all_objs = [m.obj for m in mod_modules if m.obj != None]

    # Collect impl CcInfos: own deps + inherited from module_deps via transitive_link_cc_infos.
    # These carry link-time objects from cpp_module_compile_srcs targets (e.g. _impl targets)
    # without re-linking the module archive itself, so no duplicate symbols arise.
    impl_cc_infos = [dep[CcInfo] for dep in ctx.attr.deps if CcInfo in dep]
    for dep in ctx.attr.module_deps:
        if CppModuleInfo in dep:
            tl = dep[CppModuleInfo].transitive_link_cc_infos
            if tl != None:
                impl_cc_infos.append(tl)
    merged_impl = cc_common.merge_cc_infos(cc_infos = impl_cc_infos) if impl_cc_infos else None

    if not all_objs:
        merged = cc_common.merge_cc_infos(cc_infos = impl_cc_infos)
        return [merged, CppModuleInfo(
            module_name        = "",
            pcm                = None,
            obj                = None,
            transitive_modules = mod_modules,
            transitive_hdrs    = depset(transitive = mod_hdrs),
            transitive_includes = mod_includes,
            transitive_defines  = mod_defines,
            transitive_link_cc_infos = merged_impl,
        ), DefaultInfo(files = depset([]))]

    cc_toolchain = find_cpp_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx                  = ctx,
        cc_toolchain         = cc_toolchain,
        requested_features   = ctx.features,
        unsupported_features = ctx.disabled_features,
    )

    static_lib = _create_static_lib(
        ctx, cc_toolchain, feature_configuration, all_objs, ctx.label.name + ".a")

    linker_input = cc_common.create_linker_input(
        owner     = ctx.label,
        libraries = depset([cc_common.create_library_to_link(
            actions               = ctx.actions,
            feature_configuration = feature_configuration,
            cc_toolchain          = cc_toolchain,
            static_library        = static_lib,
            alwayslink            = True,
        )]),
    )
    linking_context = cc_common.create_linking_context(
        linker_inputs = depset([linker_input]))

    all_hdrs = mod_hdrs + cc_hdrs
    all_inc  = mod_includes + [i for i in cc_includes if i not in mod_includes]

    compilation_context = cc_common.create_compilation_context(
        headers  = depset(transitive = all_hdrs),
        includes = depset(all_inc),
        defines  = depset(mod_defines),
    )
    my_cc_info  = CcInfo(
        compilation_context = compilation_context,
        linking_context     = linking_context,
    )
    final_cc_info = cc_common.merge_cc_infos(cc_infos = [my_cc_info] + impl_cc_infos)

    return [
        final_cc_info,
        CppModuleInfo(
            module_name        = "",
            pcm                = None,
            obj                = None,
            transitive_modules = mod_modules,
            transitive_hdrs    = depset(transitive = mod_hdrs),
            transitive_includes = mod_includes,
            transitive_defines  = mod_defines,
            transitive_link_cc_infos = merged_impl,
        ),
        DefaultInfo(files = depset(all_objs + [static_lib])),
    ]

cpp_module_group = rule(
    implementation = _cpp_module_group_impl,
    attrs = {
        "module_deps": attr.label_list(providers = [CppModuleInfo]),
        "deps":        attr.label_list(providers = [CcInfo]),
    } | CC_TOOLCHAIN_ATTRS,
    toolchains = use_cc_toolchain(),
    fragments  = ["cpp"],
)

# ---------------------------------------------------------------------------
# cpp_module_compile_srcs rule  (compiles .cpp files that use modules)
# ---------------------------------------------------------------------------

def _cpp_module_compile_srcs_impl(ctx):
    compiler, cc_toolchain, feature_configuration, is_gcc = _get_compiler_and_toolchain(ctx)

    mod_modules, mod_hdrs, mod_includes, mod_defines = _collect_module_info(ctx.attr.module_deps)
    cc_hdrs, cc_includes = _collect_cc_info(ctx.attr.deps)

    all_includes = mod_includes + ctx.attr.includes
    for inc in cc_includes:
        if inc not in all_includes:
            all_includes.append(inc)
    all_defines = mod_defines + ctx.attr.defines

    include_flags = ["-I" + inc for inc in all_includes]
    define_flags  = ["-D" + d   for d   in all_defines]
    all_hdrs      = mod_hdrs + cc_hdrs

    objs = []
    for src in ctx.files.srcs:
        obj = ctx.actions.declare_file(
            src.basename.replace("/", "_") + "." + ctx.label.name + ".o")
        # Per-file: look up precise imports from the pre-scanned dep map so each
        # compile action only waits for the modules it actually needs.
        direct_imports = FILE_DIRECT_DEPS.get(src.short_path)
        needed_modules = _compute_needed_modules(mod_modules, direct_imports)
        if is_gcc:
            safe_src = src.short_path.replace("/", "_").replace(".", "_")
            mapper, dep_gcms = _gcc_write_mapper(
                ctx, None, None, needed_modules, suffix = "_src_" + safe_src)
            ctx.actions.run(
                inputs = depset(
                    direct    = [src, mapper] + dep_gcms,
                    transitive = all_hdrs + [cc_toolchain.all_files],
                ),
                outputs    = [obj],
                executable = compiler,
                env        = _GCC_ENV,
                arguments  = [
                    "-std=c++23", "-fmodules-ts",
                    "-fmodule-mapper=" + mapper.path,
                    "-c", src.path, "-o", obj.path,
                ] + include_flags + define_flags + ctx.attr.copts,
                mnemonic         = "CppModuleCompileSrc",
                progress_message = "Compiling {} with modules".format(src.short_path),
            )
        else:
            module_flags, module_pcm_files = _module_flags(needed_modules)
            ctx.actions.run(
                inputs = depset(
                    direct    = [src] + module_pcm_files,
                    transitive = all_hdrs + [cc_toolchain.all_files],
                ),
                outputs    = [obj],
                executable = compiler,
                arguments  = [
                    "-std=c++23",
                ] + _stdlib_flags(ctx, is_gcc) + ["-c",
                ] + module_flags + include_flags + define_flags + ctx.attr.copts + [
                    src.path, "-o", obj.path,
                ],
                mnemonic         = "CppModuleCompileSrc",
                progress_message = "Compiling {} with modules".format(src.short_path),
            )
        objs.append(obj)

    cc_info_list = [dep[CcInfo] for dep in ctx.attr.deps if CcInfo in dep]

    if not objs:
        merged = cc_common.merge_cc_infos(cc_infos = cc_info_list)
        return [merged, DefaultInfo(files = depset([]))]

    static_lib = _create_static_lib(
        ctx, cc_toolchain, feature_configuration, objs, ctx.label.name + ".a")

    linker_input = cc_common.create_linker_input(
        owner     = ctx.label,
        libraries = depset([cc_common.create_library_to_link(
            actions               = ctx.actions,
            feature_configuration = feature_configuration,
            cc_toolchain          = cc_toolchain,
            static_library        = static_lib,
            alwayslink            = True,
        )]),
    )
    linking_context = cc_common.create_linking_context(
        linker_inputs = depset([linker_input]))
    merged = cc_common.merge_cc_infos(cc_infos = cc_info_list)
    my_cc_info = CcInfo(
        compilation_context = cc_common.create_compilation_context(),
        linking_context     = linking_context,
    )

    return [
        cc_common.merge_cc_infos(cc_infos = [my_cc_info, merged]),
        DefaultInfo(files = depset(objs + [static_lib])),
    ]

cpp_module_compile_srcs = rule(
    implementation = _cpp_module_compile_srcs_impl,
    attrs = {
        "srcs":        attr.label_list(allow_files = [".cpp", ".cc"]),
        "module_deps": attr.label_list(providers = [CppModuleInfo]),
        "deps":        attr.label_list(providers = [CcInfo]),
        "includes":    attr.string_list(),
        "defines":     attr.string_list(),
        "copts":       attr.string_list(),
    } | CC_TOOLCHAIN_ATTRS,
    toolchains = use_cc_toolchain(),
    fragments  = ["cpp"],
)

# ---------------------------------------------------------------------------
# cc_module_bundle rule  (self-contained archive: module objects + cpp objects)
#
# Compiles .cpp srcs against the given module_deps, then archives all module
# objects AND the compiled .cpp objects into a single .a.  Inherits impl
# CcInfos from module_deps via transitive_link_cc_infos so that cc_binary /
# cc_test only needs deps=[":this_target"] — no separate module group dep.
# ---------------------------------------------------------------------------

def _cc_module_bundle_impl(ctx):
    compiler, cc_toolchain, feature_configuration, is_gcc = _get_compiler_and_toolchain(ctx)

    mod_modules, mod_hdrs, mod_includes, mod_defines = _collect_module_info(ctx.attr.module_deps)
    cc_hdrs, cc_includes = _collect_cc_info(ctx.attr.deps)

    all_includes = mod_includes + ctx.attr.includes
    for inc in cc_includes:
        if inc not in all_includes:
            all_includes.append(inc)
    all_defines = mod_defines + ctx.attr.defines
    all_hdrs    = mod_hdrs + cc_hdrs

    include_flags = ["-I" + inc for inc in all_includes]
    define_flags  = ["-D" + d   for d   in all_defines]

    # Compile any inline .ixx module interface files.
    local_modules = []
    for ixx in ctx.files.mods:
        module_name = MODULE_SOURCES.get(ixx.short_path)
        if module_name == None:
            fail("cc_module_bundle: cannot find module name for '{}'. ".format(ixx.short_path) +
                 "Ensure the file is scanned and bazel clean --expunge has been run.")

        direct_deps_list = MODULE_DIRECT_DEPS.get(module_name)
        safe_name = module_name.replace(".", "_").replace(":", "__")
        if is_gcc:
            gcm = ctx.actions.declare_file(ctx.label.name + "_" + safe_name + ".gcm")
            obj = ctx.actions.declare_file(ctx.label.name + "_" + safe_name + ".gcm.o")
            needed = _compute_needed_modules(mod_modules + local_modules, direct_deps_list)
            mapper, dep_gcms = _gcc_write_mapper(
                ctx, module_name, gcm, needed, suffix = "_" + safe_name)
            ctx.actions.run(
                inputs = depset(
                    direct    = [ixx, mapper] + dep_gcms,
                    transitive = all_hdrs + [cc_toolchain.all_files],
                ),
                outputs    = [gcm],
                executable = compiler,
                env        = _GCC_ENV,
                arguments  = [
                    "-std=c++23", "-fmodules-ts",
                    "-fmodule-mapper=" + mapper.path,
                    "-fmodule-only", "-x", "c++", "-c", ixx.path,
                ] + include_flags + define_flags + ctx.attr.copts,
                mnemonic         = "CppModulePrecompile",
                progress_message = "Precompiling {}".format(ixx.short_path),
            )
            ctx.actions.run(
                inputs = depset(
                    direct    = [ixx, mapper, gcm] + dep_gcms,
                    transitive = all_hdrs + [cc_toolchain.all_files],
                ),
                outputs    = [obj],
                executable = compiler,
                env        = _GCC_ENV,
                arguments  = [
                    "-std=c++23", "-fmodules-ts",
                    "-fmodule-mapper=" + mapper.path,
                    "-x", "c++", "-c", ixx.path, "-o", obj.path,
                ] + include_flags + define_flags + ctx.attr.copts,
                mnemonic         = "CppModuleCompileObj",
                progress_message = "Compiling {}".format(ixx.short_path),
            )
            pcm = gcm
        else:
            stdlib = _stdlib_flags(ctx, is_gcc)
            needed = _compute_needed_modules(mod_modules + local_modules, direct_deps_list)
            mod_flags, mod_pcm_files = _module_flags(needed)
            pcm = ctx.actions.declare_file(ctx.label.name + "_" + safe_name + ".pcm")
            ctx.actions.run(
                inputs = depset(
                    direct    = [ixx] + mod_pcm_files,
                    transitive = all_hdrs + [cc_toolchain.all_files],
                ),
                outputs    = [pcm],
                executable = compiler,
                arguments  = [
                    "-std=c++23",
                    "-x", "c++-module", "--precompile",
                ] + stdlib + mod_flags + include_flags + define_flags + ctx.attr.copts + [
                    ixx.path, "-o", pcm.path,
                ],
                mnemonic         = "CppModulePrecompile",
                progress_message = "Precompiling {}".format(ixx.short_path),
            )
            obj = ctx.actions.declare_file(ctx.label.name + "_" + safe_name + ".pcm.o")
            ctx.actions.run(
                inputs = depset(
                    direct    = [pcm] + mod_pcm_files,
                    transitive = [cc_toolchain.all_files],
                ),
                outputs    = [obj],
                executable = compiler,
                arguments  = ["-std=c++23", "-c", pcm.path, "-o", obj.path],
                mnemonic         = "CppModuleCompilePcm",
                progress_message = "Compiling {}".format(pcm.short_path),
            )
        local_modules.append(struct(
            name = module_name, pcm = pcm, obj = obj, direct_deps = direct_deps_list))

    all_mod_modules = mod_modules + local_modules

    # Compile each .cpp src using only the modules it actually needs (BFS).
    cpp_objs = []
    for src in ctx.files.srcs:
        obj = ctx.actions.declare_file(
            src.basename.replace("/", "_") + "." + ctx.label.name + ".o")
        direct_imports = FILE_DIRECT_DEPS.get(src.short_path)
        needed_modules = _compute_needed_modules(all_mod_modules, direct_imports)
        if is_gcc:
            safe_src = src.short_path.replace("/", "_").replace(".", "_")
            mapper, dep_gcms = _gcc_write_mapper(
                ctx, None, None, needed_modules, suffix = "_src_" + safe_src)
            ctx.actions.run(
                inputs = depset(
                    direct    = [src, mapper] + dep_gcms,
                    transitive = all_hdrs + [cc_toolchain.all_files],
                ),
                outputs    = [obj],
                executable = compiler,
                env        = _GCC_ENV,
                arguments  = [
                    "-std=c++23", "-fmodules-ts",
                    "-fmodule-mapper=" + mapper.path,
                    "-c", src.path, "-o", obj.path,
                ] + include_flags + define_flags + ctx.attr.copts,
                mnemonic         = "CppModuleCompileSrc",
                progress_message = "Compiling {} with modules".format(src.short_path),
            )
        else:
            module_flags, module_pcm_files = _module_flags(needed_modules)
            ctx.actions.run(
                inputs = depset(
                    direct    = [src] + module_pcm_files,
                    transitive = all_hdrs + [cc_toolchain.all_files],
                ),
                outputs    = [obj],
                executable = compiler,
                arguments  = [
                    "-std=c++23",
                ] + _stdlib_flags(ctx, is_gcc) + ["-c",
                ] + module_flags + include_flags + define_flags + ctx.attr.copts + [
                    src.path, "-o", obj.path,
                ],
                mnemonic         = "CppModuleCompileSrc",
                progress_message = "Compiling {} with modules".format(src.short_path),
            )
        cpp_objs.append(obj)

    # Collect impl CcInfos: own deps + transitive_link_cc_infos from module_deps.
    impl_cc_infos = [dep[CcInfo] for dep in ctx.attr.deps if CcInfo in dep]
    for dep in ctx.attr.module_deps:
        if CppModuleInfo in dep:
            tl = dep[CppModuleInfo].transitive_link_cc_infos
            if tl != None:
                impl_cc_infos.append(tl)

    # Archive all module objects (transitive + local) + compiled cpp objects into one .a.
    all_objs = [m.obj for m in all_mod_modules if m.obj != None] + cpp_objs

    if not all_objs:
        return [
            cc_common.merge_cc_infos(cc_infos = impl_cc_infos),
            DefaultInfo(files = depset([])),
        ]

    static_lib = _create_static_lib(
        ctx, cc_toolchain, feature_configuration, all_objs, ctx.label.name + ".a")

    linker_input = cc_common.create_linker_input(
        owner     = ctx.label,
        libraries = depset([cc_common.create_library_to_link(
            actions               = ctx.actions,
            feature_configuration = feature_configuration,
            cc_toolchain          = cc_toolchain,
            static_library        = static_lib,
            alwayslink            = True,
        )]),
    )
    linking_context = cc_common.create_linking_context(
        linker_inputs = depset([linker_input]))

    compilation_context = cc_common.create_compilation_context(
        headers  = depset(transitive = all_hdrs),
        includes = depset(all_includes),
        defines  = depset(all_defines),
    )
    my_cc_info    = CcInfo(compilation_context = compilation_context,
                           linking_context     = linking_context)
    final_cc_info = cc_common.merge_cc_infos(cc_infos = [my_cc_info] + impl_cc_infos)

    return [final_cc_info, DefaultInfo(files = depset(all_objs + [static_lib]))]

cc_module_bundle = rule(
    implementation = _cc_module_bundle_impl,
    attrs = {
        "srcs":        attr.label_list(allow_files = [".cpp", ".cc"]),
        "mods":        attr.label_list(allow_files = [".ixx"]),
        "module_deps": attr.label_list(providers = [CppModuleInfo]),
        "deps":        attr.label_list(providers = [CcInfo]),
        "includes":    attr.string_list(),
        "defines":     attr.string_list(),
        "copts":       attr.string_list(),
    } | CC_TOOLCHAIN_ATTRS,
    toolchains = use_cc_toolchain(),
    fragments  = ["cpp"],
)
