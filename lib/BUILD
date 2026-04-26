load("@rules_cc//cc:defs.bzl", "cc_library")
load("//bazel:cpp_module.bzl", "cpp_module")
load("//bazel:std_module.bzl", "std_module")
load("//bazel:arc_gen.bzl", "arc_gen_headers")

arc_gen_headers(
    name = "arc_headers_gen",
    srcs = [
        "include/arc/traits/datastore.hxx.arc",
        "include/arc/traits/peer.hxx.arc",
        "include/arc/traits/dynamic_node.hxx.arc",
        "include/arc/traits/scheduler.hxx.arc",
        "include/arc/traits/detail/spy_base.hxx.arc",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "arc_headers",
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
    visibility = ["//visibility:public"],
)

std_module(
    name = "std_module",
    visibility = ["//visibility:public"],
)

cpp_module(
    name = "arc_module",
    module_name = "arc",
    src = "modules/arc.ixx",
    module_deps = [":std_module"],
    deps = [":arc_headers"],
    defines = ["ARC_IMPORT_STD=1"],
    visibility = ["//visibility:public"],
)
