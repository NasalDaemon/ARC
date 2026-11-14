load("@rules_python//python:defs.bzl", "py_binary", "py_library", "py_test")

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

py_library(
    name = "test_support",
    srcs = ["tests/support.py"],
    imports = ["tests"],
    testonly = True,
)

GENERATOR_TESTS = [
    "annotations",
    "nodiscard",
    "parser_cache",
    "requirements",
    "trunk_declarations",
    "trunk_links",
]

[py_test(
    name = "generator_test_" + area,
    srcs = ["tests/test_" + area + ".py"],
    main = "tests/test_" + area + ".py",
    args = ["$(rootpath :generator)"],
    data = [":generator"],
    deps = [":test_support"],
) for area in GENERATOR_TESTS]

test_suite(
    name = "generator_test",
    tests = [":generator_test_" + area for area in GENERATOR_TESTS],
)
