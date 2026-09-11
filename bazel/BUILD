# Bazel rule definitions

load("@rules_cc//cc:defs.bzl", "cc_library")
load("@arc_cc_info//:cc_info.bzl", "LINKOPTS")

# The detected compiler's link flags. Every cpp_module depends on this one
# target, so a binary gets them once however many modules it links.
cc_library(
    name = "cc_link_flags",
    linkopts = LINKOPTS,
    visibility = ["//visibility:public"],
)
