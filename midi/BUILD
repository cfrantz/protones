package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fti",
    hdrs = ["fti.h"],
    srcs = ["fti.cc"],
    deps = [
        "//util:file",
        "//util:status",
        "//proto:fti",
    ],
)

cc_binary(
    name = "fti_load",
    srcs = ["fti_load.cc"],
    deps = [
        ":fti",
        "//util:logging",
    ],
)

cc_library(
    name = "midi",
    srcs = ["midi.cc"],
    hdrs = ["midi.h"],
    deps = [
        "//nes:base",
        "//nes:nes-interface",
        "//nes:mem",
        "//proto:midi",
        "//proto:fti",
        "//util:file",
        "@rtmidi_git//:rtmidi",
    ],
)
