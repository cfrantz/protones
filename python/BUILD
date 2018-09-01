package(default_visibility=["//visibility:public"])

cc_library(
    name = "protones",
    srcs = ["protones.cc"],
    deps = [
        "//nes:nes",
        "//nes:nes-interface",
##        ":apu",
##        ":cartridge",
##        ":controller",
##        ":cpu6502",
##        ":mapper",
##        ":mapper-lib",
##        ":mem",
##        ":ppu",
        "@pybind11_git//:pybind11",
    ],
    alwayslink = 1,
)
