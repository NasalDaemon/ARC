"""Rule to compile and expose the C++23 std module using the CC toolchain."""

load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc:action_names.bzl", "ACTION_NAMES")
load("@rules_cc//cc:find_cc_toolchain.bzl", "CC_TOOLCHAIN_ATTRS", "use_cc_toolchain")
load("//bazel:cpp_module.bzl", "CppModuleInfo", "toolchain_cxxopts")
load("@arc_cc_info//:cc_info.bzl", "CLANG_STD_CPPM", "CXXOPTS", "GCC_STD_CPPM")


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

    # Hand-assembled compile actions, given the same flags as every other module
    # (toolchain, detected, --cxxopt, in that order). The std BMI must agree with
    # its importers on everything that shapes it: a library configuration macro
    # (_LIBCPP_HARDENING_MODE, _GLIBCXX_ASSERTIONS) missing here is silently
    # ignored by every `import std;`, since the importer's definition cannot
    # reach an already-built BMI. Without the toolchain's flags std.o would also
    # be the one -O0 object in an otherwise -c opt build.
    #
    # _FORTIFY_SOURCE is dropped: it is in the toolchain's -c opt flags, and it
    # makes glibc redeclare snprintf and friends as fortify overloads with
    # internal linkage, which libc++'s std.cppm then cannot export ("using
    # declaration referring to 'snprintf' with internal linkage cannot be
    # exported"). The -U the toolchain pairs it with is kept, so the std module
    # is simply built without fortification.
    cxxopts = [
        opt
        for opt in toolchain_cxxopts(cc_toolchain, feature_configuration) + CXXOPTS + ctx.fragments.cpp.cxxopts
        if not opt.startswith("-D_FORTIFY_SOURCE")
    ]

    is_gcc = "gcc" in cc_toolchain.compiler and "clang" not in cc_toolchain.compiler
    use_libcxx = not is_gcc and "-stdlib=libc++" in cxxopts
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
                "-fmodules-ts",
                "-fmodule-mapper=" + mapper_file.path,
                "-fmodule-only", "-x", "c++", "-c", std_cppm,
            ] + cxxopts,
            env = {"SOURCE_DATE_EPOCH": "0"},
            inputs = depset(direct = [mapper_file], transitive = [cc_toolchain.all_files]),
            outputs = [gcm],
            mnemonic = "CppCompileStdModulePrecompile",
            progress_message = "Precompiling std module (GCC)",
        )
        ctx.actions.run(
            executable = compiler,
            arguments = [
                "-fmodules-ts",
                "-fmodule-mapper=" + mapper_file.path,
                "-x", "c++", "-c", std_cppm, "-o", obj.path,
            ] + cxxopts,
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
                "-x", "c++-module", "--precompile",
            ] + cxxopts + [
                # After the user's flags, so -Wall/-Werror cannot re-enable them:
                # std.cppm declares the reserved `std` module and exports
                # deprecated names by design.
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
            arguments = ["-c", pcm.path, "-o", obj.path] + cxxopts,
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
