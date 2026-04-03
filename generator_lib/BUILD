load("@rules_python//python:defs.bzl", "py_binary")

py_binary(
    name = "generator",
    srcs = ["generate.py"],
    main = "generate.py",
    data = glob(["*.lark", "*.jinja"]),
    deps = [
        "@arc_gen_deps//jinja2",
        "@arc_gen_deps//lark",
        "@arc_gen_deps//markupsafe",
    ],
    visibility = ["//visibility:public"],
)
