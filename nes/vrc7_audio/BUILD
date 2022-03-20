package(default_visibility = ["//visibility:public"])
cc_library(
    name = "vrc7_audio",
    hdrs = ["vrc7_audio.h"],
    deps = [
#        ":emu2413",
        ":dsa_emu2413",
#        ":ym2413",
    ],
)

cc_library(
    name = "emu2413",
    defines = ["VRC7_AUDIO_EMU2413"],
    hdrs = ["emu2413.h"],
    srcs = ["emu2413.c"],
)

cc_library(
    name = "dsa_emu2413",
    defines = ["VRC7_AUDIO_DSA_EMU2413"],
    hdrs = ["dsa_emu2413.h"],
    srcs = ["dsa_emu2413.c"],
)

cc_library(
    name = "ym2413",
    defines = ["VRC7_AUDIO_YM2413"],
    hdrs = ["ym2413.h"],
    srcs = ["ym2413.c"],
)
