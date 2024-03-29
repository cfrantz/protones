package(default_visibility = ["//visibility:public"])

cc_library(
    name = "fti",
    srcs = ["fti.cc"],
    hdrs = ["fti.h"],
    deps = [
        "//proto:fti",
        "//util:file",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
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
        "//nes:mem",
        "//nes:nes-interface",
        "//proto:fti",
        "//proto:midi",
        "//util:file",
        "@rtmidi_git//:rtmidi",
    ],
)

cc_binary(
    name = "midi_convert",
    srcs = ["midi_convert.cc"],
    deps = [
        "//proto:midi_convert",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/flags:usage",
        "@midifile_git//:midifile",
    ],
)

cc_binary(
    name = "proto_player",
    srcs = ["proto_player.cc"],
    deps = [
        "//proto:midi_convert",
        "//util:file",
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/flags:usage",
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
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
        "@com_google_absl//absl/flags:usage",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/strings:str_format",
    ],
)
