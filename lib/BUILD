load("@rules_cc//cc:defs.bzl", "cc_library")
load("//bazel:cpp_module.bzl", "cpp_module")
load("//bazel:std_module.bzl", "std_module")
load("//bazel:arc_gen.bzl", "arc_gen_headers")

# Public macro header — the only arc header consumers should #include
# when they depend on :arc_module.
cc_library(
    name = "arc_macros",
    hdrs = ["macros/include/arc/macros.hpp"],
    includes = ["macros/include"],
    visibility = ["//visibility:public"],
)

arc_gen_headers(
    name = "arc_headers_gen",
    srcs = glob(["include/arc/**/*.hxx.arc"]),
    includes = ["include"],
    visibility = ["//visibility:private"],
)

cc_library(
    name = "arc_headers_lib",
    hdrs = glob([
        "include/arc/**/*.hpp",
        "macros/include/arc/**/*.hpp",
    ]),
    includes = [
        "include",
        "macros/include",
    ],
    deps = [":arc_headers_gen"],
    strip_include_prefix = "",
    visibility = ["//visibility:private"],
)

# Internal .tpp private to //lib only — must never propagate downstream.
cc_library(
    name = "arc_headers_inc",
    hdrs = glob(["src/include/**/*.tpp"]),
    includes = ["src/include"],
    visibility = ["//visibility:private"],
)

# Header-mode arc library. Exposes the public hpp API but keeps src/include
# .tpp files compile-only (used by tracer_spy.cpp) so they don't leak.
cc_library(
    name = "arc_headers",
    srcs = ["src/tracer_spy.cpp"],
    deps = [":arc_headers_lib"],
    implementation_deps = [":arc_headers_inc"],
    visibility = ["//visibility:public"],
)

std_module(
    name = "std_module",
    visibility = ["//visibility:public"],
)

# Module-mode arc library. Consumers of :arc_module can #include
# "arc/macros.hpp" via :arc_macros only — every other arc header is
# implementation_deps and stays inside this target.
cpp_module(
    name = "arc_module",
    mods = ["modules/arc.ixx"],
    srcs = ["modules/tracer_spy.cpp"],
    module_deps = [":std_module"],
    deps = [":arc_macros"],
    implementation_deps = [":arc_headers_lib", ":arc_headers_inc"],
    defines = ["ARC_IMPORT_STD=1"],
    visibility = ["//visibility:public"],
)
