package(default_visibility = ["//visibility:public"])

cc_library(
    name = "protones",
    srcs = ["protones.cc"],
    deps = [
        "//nes",
        "//nes:nes-interface",
        "//nes:mapper",
        ## ":apu",
        ## ":cartridge",
        ## ":controller",
        ## ":cpu6502",
        ## ":mapper-lib",
        ## ":mem",
        ## ":ppu",
        "@pybind11_git//:pybind11",
    ],
    alwayslink = 1,
)
