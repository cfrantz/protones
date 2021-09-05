package(default_visibility = ["//visibility:public"])

cc_library(
    name = "bimpy",
    defines = [
        "IMGUI_DEFINE_MATH_OPERATORS",
    ],
    srcs = [
        "sources/bimpy.cpp",
        "sources/runtime_error.h",
    ],
    deps = [
        "@imgui_git//:imgui",
        "@pybind11_git//:pybind11",
    ],
    alwayslink = 1,
)
