package(default_visibility=["//visibility:public"])

cc_library(
    name = "base",
    hdrs = ["base.h"],
)

cc_library(
    name = "pbmacro",
    hdrs = ["pbmacro.h"],
    deps = [
        "//util:macros",
    ],
)

cc_library(
    name = "nes-interface",
    hdrs = [
        "nes.h",
        "mem.h"
    ],
    deps = [
        "//proto:nes",
        "//proto:config",
    ],
)

cc_library(
    name = "mapper",
    hdrs = ["mapper.h"],
    srcs = ["mapper.cc"],
    deps = [
        ":base",
        ":nes-interface",
        "//proto:mappers",
    ],
)

cc_library(
    name = "midi",
    hdrs = ["midi.h"],
    srcs = ["midi.cc"],
    deps = [
        ":base",
        "@rtmidi_git//:rtmidi",
    ],
)

cc_library(
    name = "apu",
    hdrs = [
        "apu.h",
        "apu_dmc.h",
        "apu_noise.h",
        "apu_pulse.h",
        "apu_triangle.h",
    ],
    srcs = [
        "apu.cc",
        "apu_dmc.cc",
        "apu_noise.cc",
        "apu_pulse.cc",
        "apu_triangle.cc",
    ],
    deps = [
        ":base",
        #":midi",
        ":nes-interface",
        ":pbmacro",
        "//util:os",
        "//proto:apu",
        "//external:gflags",
        "//external:imgui",
    ],
)

cc_library(
    name = "cartridge",
    hdrs = ["cartridge.h"],
    srcs = ["cartridge.cc"],
    deps = [
        ":base",
        ":nes-interface",
        "//proto:mappers",
        "//util:crc",
        "//util:file",
        "//util:os",
        "//external:gflags",
    ],
)

cc_library(
    name = "controller",
    hdrs = ["controller.h"],
    srcs = ["controller.cc"],
    deps = [
        ":base",
        ":nes-interface",
        "//proto:config",
        "//util:config",
        "//external:imgui",
    ],
)

cc_library(
    name = "cpu6502",
    hdrs = ["cpu6502.h"],
    srcs = ["cpu6502.cc"],
    deps = [
        ":base",
        ":pbmacro",
        ":nes-interface",
        "//proto:cpu6502",
        "//external:gflags",
    ],
)

cc_binary(
    name = "cpu_test",
    srcs = ["cpu_test.cc"],
    deps = [
        ":cpu6502",
        ":nes",
        ":mem",
        "//external:gflags",
    ],
    linkopts = [
        "-lSDL2",
    ],
)

cc_library(
    name = "fm2",
    srcs = ["fm2.cc"],
    hdrs = ["fm2.h"],
    deps = [
        ":base",
        ":controller",
        ":nes-interface",
        "//external:gflags",
    ],
)


cc_library(
    name = "mapper-lib",
    hdrs = [
        "mapper1.h"
    ],
    srcs = [
        "mapper1.cc",
        "mapper2.cc",
        "mapper3.cc",
        "mapper4.cc",
        "mapper7.cc",
    ],
    deps = [
        ":mapper",
        ":cartridge",
        ":ppu",
        ":pbmacro",
        "//proto:mappers",
    ],
    alwayslink = 1,
)

cc_library(
    name = "mem",
    srcs = ["mem.cc"],
    deps = [
        ":base",
        ":apu",
        ":cartridge",
        ":controller",
        ":cpu6502",
        ":mapper",
        ":nes-interface",
        ":ppu",
        "//proto:nes",
        "//proto:cpu6502",
        "//external:imgui",
    ],
)

cc_library(
    name = "nes",
    srcs = ["nes.cc"],
    deps = [
        ":base",
        ":apu",
        ":cartridge",
        ":controller",
        ":cpu6502",
        ":fm2",
        ":mapper",
        ":mapper-lib",
        ":mem",
        #":midi",
        ":nes-interface",
        ":ppu",
        "//external:imgui",
        "//proto:config",
        "//util:config",
    ],
    alwayslink = 1,
)

cc_library(
    name = "ppu",
    srcs = ["ppu.cc"],
    hdrs = ["ppu.h"],
    deps = [
        ":base",
        ":cartridge",
        ":fm2",
        ":mapper",
        ":nes-interface",
        ":pbmacro",
        "//proto:ppu",
        "//external:imgui",
    ],
)
