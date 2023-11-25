package(default_visibility = ["//visibility:public"])

cc_library(
    name = "ips",
    srcs = [
        "ips.cc",
    ],
    hdrs = [
        "ips.h",
    ],
    deps = [
        "@com_google_absl//absl/status:statusor",
    ],
)

cc_binary(
    name = "ipspatch",
    srcs = ["ipspatch.cc"],
    deps = [
        ":ips",
        "//util:file",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/flags:usage",
        "@com_google_absl//absl/status",
    ],
)
