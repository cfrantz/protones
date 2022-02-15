package(default_visibility = ["//visibility:public"])

cc_library(
    name = "system_python",
    hdrs = glob([
        "*.h",
        "**/*.h",
    ]),
    includes = ["."],
    linkopts = [
        "-lpython3.10",
    ],
)
