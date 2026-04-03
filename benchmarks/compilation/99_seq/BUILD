load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//bazel:cpp_module.bzl", "cc_module_bundle")
load("//bazel:arc_gen.bzl", "arc_module_library")

arc_module_library(
    name = "bench99_seq_lib",
    arcs = glob(["traits/*.ixx.arc", "*.ixx.arc"]),
    mods = glob(["nodes/*.ixx"]),
)

cc_module_bundle(
    name = "bench99_seq_main",
    srcs = ["main.cpp"],
    module_deps = [":bench99_seq_lib"],
)

cc_binary(
    name = "arc_bench_compile99_seq",
    deps = [":bench99_seq_main"],
)
