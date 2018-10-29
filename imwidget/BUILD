package(default_visibility=["//visibility:public"])

cc_library(
    name = "base",
    hdrs = [
        "debug_console.h",
        "imapp.h",
        "imutil.h",
        "imwidget.h",
    ],
    srcs = [
        "debug_console.cc",
        "imapp.cc",
        "imutil.cc",
    ],
    deps = [
        "//util:fpsmgr",
        "//util:gamecontrollerdb",
        "//util:imgui_sdl_opengl",
        "//util:logging",
        "//util:os",
        "//external:gflags",
        "//external:imgui",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "python_console",
    hdrs = ["python_console.h"],
    srcs = ["python_console.cc"],
    deps = [
        ":base",
        "//util:logging",
        "//external:imgui",
        "@pybind11_git//:pybind11",
    ],
)

cc_library(
    name = "error_dialog",
    hdrs = ["error_dialog.h"],
    srcs = ["error_dialog.cc"],
    deps = [
        ":base",
        "//external:imgui",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "glbitmap",
    hdrs = ["glbitmap.h"],
    srcs = ["glbitmap.cc"],
    deps = [
        "//external:imgui",
    ],
)

cc_library(
    name = "apu_debug",
    hdrs = ["apu_debug.h"],
    srcs = ["apu_debug.cc"],
    deps = [
        ":base",
        "//nes:apu",
        "//external:imgui",
    ],
)

cc_library(
    name = "controller_debug",
    hdrs = ["controller_debug.h"],
    srcs = ["controller_debug.cc"],
    deps = [
        ":base",
        "//nes:controller",
        "//nes:nes-interface",
        "//external:imgui",
    ],
)

cc_library(
    name = "mem_debug",
    hdrs = ["mem_debug.h"],
    srcs = ["mem_debug.cc"],
    deps = [
        ":base",
        "//nes:mem",
        "//external:imgui",
    ],
)

cc_library(
    name = "ppu_debug",
    hdrs = ["ppu_debug.h"],
    srcs = ["ppu_debug.cc"],
    deps = [
        ":base",
        "//nes:ppu",
        "//nes:mapper",
        "//nes:mem",
        "//nes:nes-interface",
        "//external:imgui",
    ],
)

