"""compile_fail_test: a source that compiles, and named variants of it that must not.

The check runs as a build action: the source compiles with the toolchain's flags
and its deps' headers, and each case, defined as a macro, fails with a diagnostic
matching `expect`. A case failing for any other reason fails the check, so a
broken include cannot pass for the diagnostic under test.
"""

load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain")
load("@bazel_tools//tools/build_defs/cc:action_names.bzl", "ACTION_NAMES")
load("@rules_cc//cc/common:cc_info.bzl", "CcInfo")
load("@rules_cc//cc/common:cc_common.bzl", "cc_common")
load("@rules_cc//cc:find_cc_toolchain.bzl", "CC_TOOLCHAIN_ATTRS", "use_cc_toolchain")
load("@arc_cc_info//:cc_info.bzl", "CXXOPTS")
load("//bazel:cpp_module.bzl", "toolchain_cxxopts")

_CHECK = """
set -u
stamp=$1; expect=$2; compiler=$3; count=$4; shift 4
cases=("${@:1:$count}"); shift "$count"
log=$(mktemp)
if ! "$compiler" "$@" >"$log" 2>&1; then
    echo "compile_fail_test: the source itself does not compile:"; cat "$log"; exit 1
fi
for case in "${cases[@]}"; do
    if "$compiler" "-D$case" "$@" >"$log" 2>&1; then
        echo "compile_fail_test: $case compiled; expected a diagnostic matching '$expect'"; exit 1
    fi
    if ! grep -Eq "$expect" "$log"; then
        echo "compile_fail_test: $case failed without a diagnostic matching '$expect':"; cat "$log"; exit 1
    fi
done
touch "$stamp"
"""

def _compile_fail_test_impl(ctx):
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
    compilation = cc_common.merge_compilation_contexts(
        compilation_contexts = [dep[CcInfo].compilation_context for dep in ctx.attr.deps],
    )

    flags = (toolchain_cxxopts(cc_toolchain, feature_configuration) + CXXOPTS +
             ctx.fragments.cpp.copts + ctx.fragments.cpp.cxxopts + ctx.attr.copts)
    flags += ["-D" + define for define in compilation.defines.to_list()]
    flags += ["-I" + inc for inc in compilation.includes.to_list()]
    flags += ["-iquote" + inc for inc in compilation.quote_includes.to_list()]
    flags += ["-isystem" + inc for inc in compilation.system_includes.to_list()]
    flags += ["-fno-diagnostics-color", "-fsyntax-only", ctx.file.src.path]

    stamp = ctx.actions.declare_file(ctx.label.name + ".checked")
    ctx.actions.run_shell(
        inputs = depset([ctx.file.src], transitive = [compilation.headers, cc_toolchain.all_files]),
        outputs = [stamp],
        command = _CHECK,
        arguments = [stamp.path, ctx.attr.expect, compiler, str(len(ctx.attr.cases))] + ctx.attr.cases + flags,
        mnemonic = "CompileFailCheck",
        progress_message = "Checking that {} rejects each case".format(ctx.file.src.short_path),
    )

    runner = ctx.actions.declare_file(ctx.label.name + ".sh")
    ctx.actions.write(runner, "#!/bin/sh\nexit 0\n", is_executable = True)
    return [DefaultInfo(executable = runner, runfiles = ctx.runfiles(files = [stamp]))]

compile_fail_test = rule(
    implementation = _compile_fail_test_impl,
    test = True,
    attrs = {
        "src": attr.label(allow_single_file = [".cpp", ".cc"], mandatory = True),
        "deps": attr.label_list(providers = [CcInfo]),
        # Each case is defined as a macro in a compile of its own, which must fail.
        "cases": attr.string_list(mandatory = True, allow_empty = False),
        # An extended regular expression the failing diagnostic must match.
        "expect": attr.string(mandatory = True),
        "copts": attr.string_list(),
    } | CC_TOOLCHAIN_ATTRS,
    toolchains = use_cc_toolchain(),
    fragments = ["cpp"],
)
