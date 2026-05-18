load("@rules_cc//cc:defs.bzl", "cc_library")

# Wrapper that exposes "arc/doctest.h" (quote-include form) to all test targets
cc_library(
    name = "doctest",
    hdrs = ["include/arc/doctest.h"],
    srcs = ["test_main.cpp"],
    strip_include_prefix = "include",
    deps = ["@doctest//doctest:doctest"],
    defines = ["DOCTEST_CONFIG_SUPER_FAST_ASSERTS=1"],
    visibility = ["//visibility:public"],
)
