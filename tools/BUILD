package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "pack_config",
    srcs = ["pack_config.cc"],
    deps = [
        "//proto:config",
        "//util:config",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
    ],
)
