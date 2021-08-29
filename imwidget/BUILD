package(default_visibility = ["//visibility:public"])

cc_library(
    name = "base",
    srcs = [
        "debug_console.cc",
        "imapp.cc",
        "imutil.cc",
    ],
    hdrs = [
        "debug_console.h",
        "imapp.h",
        "imutil.h",
        "imwidget.h",
    ],
    deps = [
        "//external:gflags",
        "//external:imgui",
        "//util:fpsmgr",
        "//util:gamecontrollerdb",
        "//util:imgui_sdl_opengl",
        "//util:logging",
        "//util:os",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "python_console",
    srcs = ["python_console.cc"],
    hdrs = ["python_console.h"],
    deps = [
        ":base",
        "//external:imgui",
        "//util:logging",
        "@pybind11_git//:pybind11",
    ],
)

cc_library(
    name = "error_dialog",
    srcs = ["error_dialog.cc"],
    hdrs = ["error_dialog.h"],
    deps = [
        ":base",
        "//external:imgui",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "glbitmap",
    srcs = ["glbitmap.cc"],
    hdrs = ["glbitmap.h"],
    deps = [
        "//external:imgui",
    ],
)

cc_library(
    name = "apu_debug",
    srcs = ["apu_debug.cc"],
    hdrs = ["apu_debug.h"],
    deps = [
        ":base",
        "//external:imgui",
        "//nes:apu",
    ],
)

cc_library(
    name = "controller_debug",
    srcs = ["controller_debug.cc"],
    hdrs = ["controller_debug.h"],
    deps = [
        ":base",
        "//external:imgui",
        "//nes:controller",
        "//nes:nes-interface",
    ],
)

cc_library(
    name = "mem_debug",
    srcs = ["mem_debug.cc"],
    hdrs = ["mem_debug.h"],
    deps = [
        ":base",
        "//external:imgui",
        "//nes:mem",
    ],
)

cc_library(
    name = "midi_setup",
    srcs = ["midi_setup.cc"],
    hdrs = ["midi_setup.h"],
    deps = [
        ":base",
        "//external:imgui",
        "//midi:midi",
        "//nes:nes-interface",
    ],
)

cc_library(
    name = "ppu_debug",
    srcs = ["ppu_debug.cc"],
    hdrs = ["ppu_debug.h"],
    deps = [
        ":base",
        "//external:imgui",
        "//nes:mapper",
        "//nes:mem",
        "//nes:nes-interface",
        "//nes:ppu",
    ],
)
