package(default_visibility = ["//visibility:public"])

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
        "mem.h",
        "nes.h",
    ],
    deps = [
        "//proto:config",
        "//proto:nes",
    ],
)

cc_library(
    name = "mapper",
    srcs = ["mapper.cc"],
    hdrs = ["mapper.h"],
    deps = [
        ":base",
        ":cartridge",
        ":nes-interface",
        "//proto:mappers",
    ],
)

cc_library(
    name = "apu",
    srcs = [
        "apu.cc",
        "apu_dmc.cc",
        "apu_noise.cc",
        "apu_pulse.cc",
        "apu_triangle.cc",
    ],
    hdrs = [
        "apu.h",
        "apu_dmc.h",
        "apu_noise.h",
        "apu_pulse.h",
        "apu_triangle.h",
    ],
    deps = [
        ":base",
        ":mapper",
        ":nes-interface",
        ":pbmacro",
        "//external:imgui",
        "//proto:apu",
        "//util:os",
        "@com_google_absl//absl/flags:flag",
    ],
)

cc_library(
    name = "cartridge",
    srcs = ["cartridge.cc"],
    hdrs = ["cartridge.h"],
    deps = [
        ":base",
        ":nes-interface",
        "//proto:mappers",
        "//util:crc",
        "//util:file",
        "//util:os",
        "@com_google_absl//absl/flags:flag",
    ],
)

cc_library(
    name = "controller",
    srcs = ["controller.cc"],
    hdrs = ["controller.h"],
    deps = [
        ":base",
        ":nes-interface",
        "//external:imgui",
        "//proto:config",
        "//util:config",
    ],
)

cc_library(
    name = "cpu6502",
    srcs = ["cpu6502.cc"],
    hdrs = ["cpu6502.h"],
    deps = [
        ":base",
        ":nes-interface",
        ":pbmacro",
        "//proto:cpu6502",
        "@com_google_absl//absl/flags:flag",
    ],
)

cc_binary(
    name = "cpu_test",
    srcs = ["cpu_test.cc"],
    linkopts = [
        "-lSDL2",
    ],
    deps = [
        ":cpu6502",
        ":mem",
        ":nes",
        "@com_google_absl//absl/flags:flag",
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
        "@com_google_absl//absl/flags:flag",
    ],
)

cc_library(
    name = "mapper-lib",
    srcs = [
        "mapper1.cc",
        "mapper2.cc",
        "mapper3.cc",
        "mapper4.cc",
        "mapper5.cc",
        "mapper7.cc",
        "vrc7.cc",
    ],
    hdrs = [
        "mapper1.h",
    ],
    deps = [
        ":apu",
        ":cartridge",
        ":cpu6502",
        ":mapper",
        ":pbmacro",
        ":ppu",
        "//nes/vrc7_audio",
        "//proto:mappers",
    ],
    alwayslink = 1,
)

cc_library(
    name = "mem",
    srcs = ["mem.cc"],
    deps = [
        ":apu",
        ":base",
        ":cartridge",
        ":controller",
        ":cpu6502",
        ":mapper",
        ":nes-interface",
        ":ppu",
        "//external:imgui",
        "//proto:cpu6502",
        "//proto:nes",
    ],
)

cc_library(
    name = "nes",
    srcs = ["nes.cc"],
    deps = [
        ":apu",
        ":base",
        ":cartridge",
        ":controller",
        ":cpu6502",
        ":fm2",
        ":mapper",
        ":mapper-lib",
        ":mem",
        ":nes-interface",
        ":ppu",
        "//external:imgui",
        "//midi",
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
        "//external:imgui",
        "//proto:ppu",
    ],
)
