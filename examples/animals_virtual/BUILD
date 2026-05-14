load("@rules_cc//cc:defs.bzl", "cc_binary")
load("//bazel:arc_gen.bzl", "arc_module")

# Node dependency order (each module may import those listed before it):
#   ianimal (base interface) → cow (extends ianimal) → goat (extends ianimal, cow)
#   → sheep (extends ianimal, goat) → fox, farmer, animal_facade (extend ianimal/cow)
arc_module(
    name = "av_lib",
    arcs = glob(["*.ixx.arc"]),
    mods = [
        "nodes/ianimal.ixx",
        "nodes/fox.ixx",
        "nodes/farmer.ixx",
        "nodes/cow.ixx",
        "nodes/goat.ixx",
        "nodes/sheep.ixx",
        "nodes/animal_facade.ixx",
    ],
    srcs = ["main.cpp"],
)

cc_binary(
    name = "arc_example_animals_virtual",
    deps = [":av_lib"],
)
