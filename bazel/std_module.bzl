"""Rule to compile and expose the C++23 std module using the CC toolchain."""

load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc:action_names.bzl", "ACTION_NAMES")
load("@rules_cc//cc:find_cc_toolchain.bzl", "CC_TOOLCHAIN_ATTRS", "use_cc_toolchain")
load("//bazel:cpp_module.bzl", "CppModuleInfo")
load("@arc_std_paths//:std_paths.bzl", "CLANG_STD_CPPM", "GCC_STD_CPPM")


def _std_module_impl(ctx):
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
    stdlib_flags = [] if is_gcc else [
        opt for opt in ctx.fragments.cpp.cxxopts if opt.startswith("-stdlib=")
    ]
    use_libcxx = "-stdlib=libc++" in stdlib_flags
    std_cppm = ctx.attr.std_cppm if (is_gcc or not use_libcxx) else ctx.attr.clang_std_cppm

    if not std_cppm:
        if is_gcc:
            fail("std_module: GCC_STD_CPPM is empty — bits/std.cc was not found in GCC's " +
                 "include paths. Ensure GCC 15+ is installed with its C++ headers.")
        else:
            fail("std_module: CLANG_STD_CPPM is empty — libc++ std.cppm was not found. " +
                 "Ensure libc++-dev is installed (e.g. libc++-20-dev).")

    if is_gcc:
        # GCC: two-step via module mapper.
        gcm = ctx.actions.declare_file("std.gcm")
        obj = ctx.actions.declare_file("std.o")
        mapper_file = ctx.actions.declare_file("std.modmap")
        ctx.actions.write(mapper_file, "MAPPING\nstd {}\n".format(gcm.path))
        ctx.actions.run(
            executable = compiler,
            arguments = [
                "-std=c++23", "-fmodules-ts",
                "-fmodule-mapper=" + mapper_file.path,
                "-fmodule-only", "-x", "c++", "-c", std_cppm,
            ],
            env = {"SOURCE_DATE_EPOCH": "0"},
            inputs = depset(direct = [mapper_file], transitive = [cc_toolchain.all_files]),
            outputs = [gcm],
            mnemonic = "CppCompileStdModulePrecompile",
            progress_message = "Precompiling std module (GCC)",
        )
        ctx.actions.run(
            executable = compiler,
            arguments = [
                "-std=c++23", "-fmodules-ts",
                "-fmodule-mapper=" + mapper_file.path,
                "-x", "c++", "-c", std_cppm, "-o", obj.path,
            ],
            env = {"SOURCE_DATE_EPOCH": "0"},
            inputs = depset(direct = [mapper_file, gcm], transitive = [cc_toolchain.all_files]),
            outputs = [obj],
            mnemonic = "CppCompileStdModuleCompile",
            progress_message = "Compiling std module object (GCC)",
        )
        pcm = gcm

    else:
        # Clang: two-step — precompile the selected std module source → .pcm,
        # then compile .pcm → .o. With libstdc++ it uses GCC's bits/std.cc;
        # with -stdlib=libc++ it uses clang_std_cppm (libc++ std.cppm).
        pcm = ctx.actions.declare_file("std.pcm")
        ctx.actions.run(
            executable = compiler,
            arguments = [
                "-std=c++23",
            ] + stdlib_flags + [
                "-x", "c++-module", "--precompile",
                "-Wno-reserved-module-identifier",
                "-Wno-deprecated-declarations",
                std_cppm, "-o", pcm.path,
            ],
            inputs = depset(transitive = [cc_toolchain.all_files]),
            outputs = [pcm],
            mnemonic = "CppCompileStdModulePrecompile",
            progress_message = "Precompiling std module (Clang)",
        )
        obj = ctx.actions.declare_file("std.o")
        ctx.actions.run(
            executable = compiler,
            arguments = ["-std=c++23", "-c", pcm.path, "-o", obj.path],
            inputs = depset(direct = [pcm], transitive = [cc_toolchain.all_files]),
            outputs = [obj],
            mnemonic = "CppCompileStdModuleObj",
            progress_message = "Compiling std module object (Clang)",
        )

    entry = struct(name = "std", pcm = pcm, obj = obj, direct_deps = [])
    return [
        CppModuleInfo(
            module_name = "std",
            pcm = pcm,
            obj = obj,
            transitive_modules = [entry],
            transitive_hdrs = depset([]),
            transitive_includes = [],
            transitive_defines = [],
            transitive_link_cc_infos = None,
        ),
        DefaultInfo(files = depset([pcm, obj])),
    ]


std_module = rule(
    implementation = _std_module_impl,
    attrs = {
        "std_cppm": attr.string(
            default = GCC_STD_CPPM,
            doc = "Path to std module source for GCC (bits/std.cc).",
        ),
        "clang_std_cppm": attr.string(
            default = CLANG_STD_CPPM,
            doc = "Path to libc++ std.cppm for Clang.",
        ),
    } | CC_TOOLCHAIN_ATTRS,
    toolchains = use_cc_toolchain(),
    fragments = ["cpp"],
)
