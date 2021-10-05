package(default_visibility = ["//visibility:public"])

cc_library(
    name = "gamecontrollerdb",
    srcs = ["gamecontrollerdb.cc"],
    hdrs = ["gamecontrollerdb.h"],
)

cc_library(
    name = "browser",
    srcs = [
        "browser.cc",
    ],
    hdrs = [
        "browser.h",
    ],
)

cc_library(
    name = "compress",
    srcs = [
        "compress.cc",
    ],
    hdrs = [
        "compress.h",
    ],
    linkopts = ["-lz"],
    deps = [
        "//util:logging",
        "//util:status",
    ],
)

cc_library(
    name = "config",
    hdrs = [
        "config.h",
    ],
    deps = [
        "//util:file",
        "//util:logging",
        "//util:os",
    ],
)

cc_library(
    name = "crc",
    srcs = ["crc.cc"],
    hdrs = ["crc.h"],
    linkopts = ["-lz"],
)

cc_library(
    name = "file",
    srcs = [
        "file.cc",
    ],
    hdrs = [
        "file.h",
    ],
    deps = [
        ":posix_status",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "logging",
    srcs = [
        "logging.cc",
    ],
    hdrs = [
        "logging.h",
    ],
    deps = [
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "os",
    srcs = [
        "os.cc",
    ],
    hdrs = [
        "os.h",
    ],
    deps = [
        ":logging",
        "@com_google_absl//absl/strings",
    ],
)

cc_library(
    name = "posix_status",
    srcs = [
        "posix_status.cc",
    ],
    hdrs = [
        "posix_status.h",
    ],
    deps = [
        "@com_google_absl//absl/status",
    ],
)

cc_library(
    name = "fpsmgr",
    hdrs = [
        "fpsmgr.h",
    ],
)

cc_library(
    name = "macros",
    hdrs = [
        "macros.h",
    ],
)

cc_library(
    name = "stb-tilemap-editor",
    hdrs = [
        "stb_tilemap_editor.h",
    ],
)

cc_library(
    name = "stb-perlin",
    hdrs = [
        "stb_perlin.h",
    ],
)

genrule(
    name = "gen_fontawesome_h",
    srcs = ["data/fontawesome-webfont.ttf"],
    outs = ["fontawesome.h"],
    cmd = "xxd -i $< $@",
)

cc_library(
    name = "imgui_sdl_opengl",
    srcs = [
        "fontawesome.h",
        "imgui_impl_sdl.cpp",
    ],
    hdrs = [
        "imgui_impl_sdl.h",
    ],
    deps = [
        "//external:fontawesome",
        "//external:imgui",
    ],
)
