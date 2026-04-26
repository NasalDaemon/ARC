load("@rules_cc//cc:defs.bzl", "cc_test")
load("//bazel:cpp_module.bzl", "cc_module_bundle")
load("//bazel:arc_gen.bzl", "arc_module_library")

arc_module_library(
    name = "bench99_seq_lib",
    arcs = glob(["traits/*.ixx.arc", "*.ixx.arc"]),
    mods = glob(["nodes/*.ixx"]),
    tags = ["manual"],
)

cc_module_bundle(
    name = "bench99_seq_main",
    srcs = ["main.cpp"],
    module_deps = [":bench99_seq_lib"],
    tags = ["manual"],
)

cc_test(
    name = "arc_bench_compile99_seq",
    deps = [":bench99_seq_main"],
    tags = ["manual"],
)
