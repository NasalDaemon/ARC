load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//bazel:arc_gen.bzl", "arc_module_library")

arc_module_library(
    name = "greet_lib",
    arcs = glob(["*.ixx.arc", "clusters/*.ixx.arc"]),
    mods = glob(["nodes/*.ixx"]),
    srcs = ["main.cpp"],
)

cc_binary(
    name = "arc_example_greet",
    deps = [":greet_lib"],
)
