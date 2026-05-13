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
        "module_name": "string: primary module name (empty for multi-module targets)",
        "pcm": "File: primary precompiled module (.pcm), or None",
        "obj": "File: primary object file, or None",
        "transitive_modules": "list of struct(name, pcm, obj, direct_deps): all transitive modules",
        "transitive_hdrs": "depset of File: all transitive header files",
        "transitive_includes": "list of string: include directories",
        "transitive_defines": "list of string: defines",
        "transitive_link_cc_infos": "CcInfo or None: merged link-time CcInfo, propagated through module_deps",
    },
)

# GCC embeds a timestamp in .gcm files; SOURCE_DATE_EPOCH=0 makes output deterministic.
_GCC_ENV = {"SOURCE_DATE_EPOCH": "0"}

# ---------------------------------------------------------------------------
# Dependency computation helpers
# ---------------------------------------------------------------------------

def _compute_needed_modules(all_modules, direct_imports):
    """BFS over the module dep graph to find the minimal reachable set."""
    if direct_imports == None:
        return all_modules

    by_name = {}
    for m in all_modules:
        by_name[m.name] = m

    needed   = {}
    frontier = list(direct_imports)

    for _pass in range(len(all_modules) + 1):
        if not frontier:
            break
        next_frontier = []
        for name in frontier:
            if name in needed or name not in by_name:
                continue
            m = by_name[name]
            needed[name] = m
            m_deps = getattr(m, "direct_deps", None)
            if m_deps == None:
                return list(all_modules)
            for dep in m_deps:
                if dep not in needed:
                    next_frontier.append(dep)
        frontier = next_frontier

    return list(needed.values())

def _module_flags(modules):
    """Return (-fmodule-file= flag strings, pcm file list) for Clang."""
    flags = []
    pcms  = []
    for m in modules:
        flags.append("-fmodule-file={}={}".format(m.name, m.pcm.path))
        pcms.append(m.pcm)
    return flags, pcms

def _gcc_write_mapper(ctx, module_name, gcm_output, dep_modules, suffix = ""):
    """Write a GCC module mapper file."""
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
# Static archive helper
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

# ---------------------------------------------------------------------------
# Compilation helpers (shared by cpp_module and cc_module_bundle)
# ---------------------------------------------------------------------------

def _compile_module_interface(ctx, compiler, cc_toolchain, is_gcc, src, module_name,
                              needed_modules, include_flags, define_flags, all_hdrs,
                              safe_name = None):
    """Compile a single .ixx module interface file. Returns (pcm, obj)."""
    module_flags, module_pcm_files = _module_flags(needed_modules)
    prefix = ctx.label.name if not safe_name else ctx.label.name + "_" + safe_name

    if is_gcc:
        gcm = ctx.actions.declare_file(prefix + ".gcm")
        obj = ctx.actions.declare_file(prefix + ".gcm.o")
        mapper, dep_gcms = _gcc_write_mapper(
            ctx, module_name, gcm, needed_modules,
            suffix = "" if not safe_name else "_" + safe_name)
        if ctx.attr.leaf:
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
                progress_message = "Compiling C++ module {} [{}]".format(module_name, ctx.label),
            )
        else:
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
                progress_message = "Precompiling C++ module {} [{}]".format(module_name, ctx.label),
            )
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
                progress_message = "Compiling C++ module object {}".format(prefix),
            )
        return gcm, obj
    else:
        stdlib = _stdlib_flags(ctx, is_gcc)
        pcm = ctx.actions.declare_file(prefix + ".pcm")
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
            progress_message = "Precompiling C++ module {} [{}]".format(module_name, ctx.label),
        )
        obj = ctx.actions.declare_file(prefix + ".o")
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
            progress_message = "Compiling C++ module object {}".format(prefix),
        )
        return pcm, obj

def _compile_src(ctx, compiler, cc_toolchain, is_gcc, src, all_mod_modules,
                 include_flags, define_flags, all_hdrs):
    """Compile a single .cpp source file against modules. Returns obj."""
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
    return obj

def _make_cc_info(ctx, cc_toolchain, feature_configuration, all_objs, all_hdrs,
                  all_includes, all_defines, impl_cc_infos):
    """Create CcInfo from objects + transitive impl CcInfos."""
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
    my_cc_info = CcInfo(
        compilation_context = compilation_context,
        linking_context     = linking_context,
    )
    final_cc_info = cc_common.merge_cc_infos(cc_infos = [my_cc_info] + impl_cc_infos)
    return final_cc_info, static_lib

def _make_empty_cc_info(impl_cc_infos):
    if impl_cc_infos:
        return cc_common.merge_cc_infos(cc_infos = impl_cc_infos)
    return CcInfo()

# ---------------------------------------------------------------------------
# cpp_module rule
#
# Two modes:
#   1. mods — list of .ixx files; module names resolved from MODULE_SOURCES.
#      Optional module_name override for generated sources not in the scan map.
#   2. srcs — list of .cpp files compiled against the modules.
#
# At least one of mods or srcs must be provided.
# ---------------------------------------------------------------------------

def _cpp_module_impl(ctx):
    compiler, cc_toolchain, feature_configuration, is_gcc = _get_compiler_and_toolchain(ctx)

    mod_modules, mod_hdrs, mod_includes, mod_defines = _collect_module_info(ctx.attr.module_deps)
    cc_hdrs, cc_includes = _collect_cc_info(ctx.attr.deps)
    impl_hdrs, impl_includes = _collect_cc_info(ctx.attr.implementation_deps)

    # Compile-time view: deps + implementation_deps headers/includes.
    compile_includes = mod_includes + ctx.attr.includes
    for inc in cc_includes:
        if inc not in compile_includes:
            compile_includes.append(inc)
    for inc in impl_includes:
        if inc not in compile_includes:
            compile_includes.append(inc)
    all_defines  = mod_defines + ctx.attr.defines
    compile_hdrs = mod_hdrs + cc_hdrs + impl_hdrs

    # Propagated view: only deps headers/includes (implementation_deps are compile-only).
    export_includes = mod_includes + ctx.attr.includes
    for inc in cc_includes:
        if inc not in export_includes:
            export_includes.append(inc)
    export_hdrs = mod_hdrs + cc_hdrs

    include_flags = ["-I" + inc for inc in compile_includes]
    define_flags  = ["-D" + d   for d   in all_defines]

    # Propagate transitive link CcInfos from module_deps.
    impl_cc_infos = []
    for dep in ctx.attr.module_deps:
        if CppModuleInfo in dep:
            tl = dep[CppModuleInfo].transitive_link_cc_infos
            if tl != None:
                impl_cc_infos.append(tl)
    # implementation_deps: propagate linking only, drop compilation_context.
    for dep in ctx.attr.implementation_deps:
        if CcInfo in dep:
            impl_cc_infos.append(CcInfo(linking_context = dep[CcInfo].linking_context))

    # --- Compile module interfaces from mods ---
    entries = []
    all_objs = []
    primary_pcm = None
    primary_obj = None
    multi = len(ctx.files.mods) > 1

    # Resolve (file, module_name, direct_deps) for every mod.
    mod_specs = []
    for ixx in ctx.files.mods:
        module_name = ctx.attr.module_names.get(ixx.short_path)
        if module_name == None:
            module_name = MODULE_SOURCES.get(ixx.short_path)
        if module_name == None:
            fail("cpp_module: cannot find module name for '{}'. ".format(ixx.short_path) +
                 "Ensure the file is scanned and bazel clean --expunge has been run, " +
                 "or pass module_names = {{'{}': '<name>'}} for generated sources.".format(ixx.short_path))

        direct_deps_list = ctx.attr.known_direct_deps_map.get(module_name)
        if direct_deps_list == None:
            direct_deps_list = MODULE_DIRECT_DEPS.get(module_name)

        mod_specs.append(struct(file = ixx, name = module_name, direct_deps = direct_deps_list))

    all_hdrs = compile_hdrs

    # Topo-sort so intra-batch deps compile first.
    in_batch = {s.name: s for s in mod_specs}
    ordered = []
    placed  = {}

    def _visit(spec, stack):
        if spec.name in placed:
            return
        if spec.name in stack:
            fail("cpp_module: cyclic intra-target module dep involving '{}'".format(spec.name))
        stack[spec.name] = True
        for dep in (spec.direct_deps or []):
            dep_spec = in_batch.get(dep)
            if dep_spec != None:
                _visit(dep_spec, stack)
        stack.pop(spec.name)
        placed[spec.name] = True
        ordered.append(spec)

    # Starlark forbids recursion; emulate with explicit stack.
    for root in mod_specs:
        if root.name in placed:
            continue
        work = [(root, 0)]
        for _step in range(len(mod_specs) * len(mod_specs) + 1):
            if not work:
                break
            spec, idx = work[-1]
            deps = spec.direct_deps or []
            advanced = False
            for j in range(idx, len(deps)):
                dep_spec = in_batch.get(deps[j])
                if dep_spec != None and dep_spec.name not in placed:
                    work[-1] = (spec, j + 1)
                    work.append((dep_spec, 0))
                    advanced = True
                    break
            if not advanced:
                work.pop()
                if spec.name not in placed:
                    placed[spec.name] = True
                    ordered.append(spec)

    for spec in ordered:
        needed_modules = _compute_needed_modules(mod_modules + entries, spec.direct_deps)
        safe_name = spec.name.replace(".", "_").replace(":", "__") if multi else None
        pcm, obj = _compile_module_interface(
            ctx, compiler, cc_toolchain, is_gcc, spec.file, spec.name,
            needed_modules, include_flags, define_flags, all_hdrs,
            safe_name = safe_name)
        all_objs.append(obj)
        entries.append(struct(
            name = spec.name, pcm = pcm, obj = obj, direct_deps = spec.direct_deps))
        if primary_pcm == None:
            primary_pcm = pcm
            primary_obj = obj

    # --- Compile implementation sources from srcs ---
    all_mod_modules = mod_modules + entries
    for src_file in ctx.files.srcs:
        obj = _compile_src(ctx, compiler, cc_toolchain, is_gcc, src_file,
                           all_mod_modules, include_flags, define_flags, all_hdrs)
        all_objs.append(obj)

    # --- Archive and return ---
    if all_objs:
        final_cc_info, static_lib = _make_cc_info(
            ctx, cc_toolchain, feature_configuration, all_objs, export_hdrs,
            export_includes, all_defines, impl_cc_infos)
        return [
            final_cc_info,
            CppModuleInfo(
                module_name        = entries[0].name if entries else "",
                pcm                = primary_pcm,
                obj                = primary_obj,
                transitive_modules = mod_modules + entries,
                transitive_hdrs    = depset(transitive = export_hdrs),
                transitive_includes = export_includes,
                transitive_defines  = all_defines,
                transitive_link_cc_infos = final_cc_info,
            ),
            DefaultInfo(files = depset(all_objs + [static_lib])),
        ]

    return [
        _make_empty_cc_info(impl_cc_infos),
        CppModuleInfo(
            module_name        = "",
            pcm                = None,
            obj                = None,
            transitive_modules = mod_modules,
            transitive_hdrs    = depset(transitive = export_hdrs),
            transitive_includes = export_includes,
            transitive_defines  = all_defines,
            transitive_link_cc_infos = None,
        ),
        DefaultInfo(files = depset([])),
    ]

cpp_module = rule(
    implementation = _cpp_module_impl,
    attrs = {
        "mods":                  attr.label_list(allow_files = [".ixx"]),
        "srcs":                  attr.label_list(allow_files = [".cpp", ".cc"]),
        # Per-file MODULE_SOURCES overrides for generated .ixx outputs whose
        # short_path is not in the scanner map. Key: file short_path.
        "module_names":          attr.string_dict(),
        "module_deps":           attr.label_list(providers = [CppModuleInfo]),
        "deps":                  attr.label_list(providers = [CcInfo]),
        # Compile-only deps. Headers and includes are visible during compile,
        # but NOT propagated in the returned CcInfo (linking_context still flows).
        "implementation_deps":   attr.label_list(providers = [CcInfo]),
        "includes":              attr.string_list(),
        "defines":               attr.string_list(),
        "copts":                 attr.string_list(),
        # Per-module MODULE_DIRECT_DEPS overrides. Key: module name.
        "known_direct_deps_map": attr.string_list_dict(),
        # Leaf modules: skip two-step GCC precompile (single pass).
        "leaf":                  attr.bool(default = False),
    } | CC_TOOLCHAIN_ATTRS,
    toolchains = use_cc_toolchain(),
    fragments  = ["cpp"],
)

