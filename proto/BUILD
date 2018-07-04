package(default_visibility=["//visibility:public"])

proto_library(
    name = "cpu6502_proto",
    srcs = [
        "cpu6502.proto",
    ],
)
cc_proto_library(
    name = "cpu6502",
    deps = [":cpu6502_proto"],
)

proto_library(
    name = "apu_proto",
    srcs = [
        "apu.proto",
    ],
)
cc_proto_library(
    name = "apu",
    deps = [":apu_proto"],
)

proto_library(
    name = "ppu_proto",
    srcs = [
        "ppu.proto",
    ],
)
cc_proto_library(
    name = "ppu",
    deps = [":ppu_proto"],
)

proto_library(
    name = "mappers_proto",
    srcs = [
        "mappers.proto",
    ],
)
cc_proto_library(
    name = "mappers",
    deps = [":mappers_proto"],
)

proto_library(
    name = "nes_proto",
    srcs = [
        "nes.proto",
    ],
    deps = [
        ":cpu6502_proto",
        ":apu_proto",
        ":ppu_proto",
        ":mappers_proto",
    ],
)
cc_proto_library(
    name = "nes",
    deps = [":nes_proto"],
)

proto_library(
    name = "config_proto",
    srcs = [
        "config.proto",
        "controller.proto",
    ],
    deps = [
    ],
)

cc_proto_library(
    name = "config",
    deps = [":config_proto"],
)
