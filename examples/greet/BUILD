load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//bazel:arc_gen.bzl", "arc_module")

arc_module(
    name = "greet_lib",
    arcs = glob(["**/*.ixx.arc"]),
    mods = glob(["**/*.ixx"]),
    srcs = ["main.cpp"],
)

cc_binary(
    name = "arc_example_greet",
    deps = [":greet_lib"],
)
