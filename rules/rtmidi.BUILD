package(default_visibility = ["//visibility:public"])

cc_library(
    name = "rtmidi",
	linkopts = [
        "-lasound",
        "-lpthread",
    ],
    copts = [
        "-D__LINUX_ALSA__=1",
        "-D__UNIX_JACK___=1",
        "-D__RTMIDI_DUMMY___=1",
    ],
    hdrs = [
        "RtMidi.h",
    ],
    srcs = [
        "RtMidi.cpp",
    ],
)
