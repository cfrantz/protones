package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fti",
    hdrs = ["fti.h"],
    srcs = ["fti.cc"],
    deps = [
        "//util:file",
        "//util:status",
        "//proto:fti",
        "@com_google_absl//absl/strings",
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
        ":fti",
        "//nes:base",
        "//nes:nes-interface",
        "//nes:mem",
        "//proto:midi",
        "//proto:fti",
        "//util:file",
        "@rtmidi_git//:rtmidi",
    ],
)

cc_binary(
    name = "midi_convert",
    srcs = ["midi_convert.cc"],
    deps = [
        "//proto:midi_convert",
        "//external:gflags",
        "@midifile_git//:midifile",
    ],
)

cc_binary(
    name = "proto_player",
    srcs = ["proto_player.cc"],
    deps = [
        "//proto:midi_convert",
        "//util:file",
        "//external:gflags",
        "@rtmidi_git//:rtmidi",
    ],
)

cc_binary(
    name = "proto_convert",
    srcs = ["proto_convert.cc"],
    deps = [
        ":fti",
        "//proto:fti",
        "//proto:midi",
        "//proto:midi_convert",
        "//util:file",
        "//external:gflags",
        "@com_google_absl//absl/strings",
    ],
)
