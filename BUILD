package(default_visibility=["//visibility:public"])
load("@mxebzl//tools/windows:rules.bzl", "pkg_winzip")

config_setting(
    name = "windows",
    values = {
        "crosstool_top": "@mxebzl//tools/windows:toolchain",
    }
)

genrule(
    name = "make_version",
    outs = ["version.h"],
    cmd = """sed -e 's/ / "/g' -e 's/$$/"\\n/g' -e "s/^/#define /g" bazel-out/volatile-status.txt > $@""",
    stamp = 1,
)

cc_library(
    name = "app",
    linkopts = [
        "-lSDL2main",
        "-lSDL2",
        "-lSDL2_image",
        "-lSDL2_mixer",
        "-lSDL2_gfx",
    ],
    hdrs = [
        "app.h",
        "version.h",
    ],
    srcs = [
        "app.cc",
    ],
    deps = [
        "//imwidget:base",
        "//imwidget:apu_debug",
        "//imwidget:controller_debug",
        "//imwidget:mem_debug",
        "//imwidget:ppu_debug",
        "//imwidget:error_dialog",
        "//nes:nes",
        "//proto:config",
        "//util:browser",
        "//util:config",
        "//util:fpsmgr",
        "//util:imgui_sdl_opengl",
        "//util:os",
        "//util:logging",
        "//external:gflags",

        # TODO(cfrantz): on ubuntu 16 with MIR, there is a library conflict
        # between MIR (linked with protobuf 2.6.1) and this program,
        # which builds with protbuf 3.x.x.  A temporary workaround is to
        # not link with nfd (native-file-dialog).
        "//external:nfd",
        "@com_google_absl//absl/memory",
    ],
)

filegroup(
    name = "content",
    srcs = glob(["content/*.textpb"]),
)

genrule(
    name = "make_protones_config",
    srcs = [
        "protones.textpb",
        ":content",
    ],
    outs = ["protones_config.h"],
    cmd = "$(location //tools:pack_config) --config $(location protones.textpb)" +
          " --symbol kProtonesCfg > $(@)",
    tools = ["//tools:pack_config"],
)

cc_binary(
    name = "protones",
    linkopts = select({
        ":windows": [
            "-lpthread",
            "-lm",
            "-lopengl32",
            "-ldinput8",
            "-ldxguid",
            "-ldxerr8",
            "-luser32",
            "-lgdi32",
            "-lwinmm",
            "-limm32",
            "-lole32",
            "-loleaut32",
            "-lshell32",
            "-lversion",
            "-luuid",

        ],
        "//conditions:default": [
            "-lpthread",
            "-lm",
            "-lGL",
        ],
    }),
    srcs = [
        "protones_config.h",
        "main.cc",
    ],
    deps = [
        ":app",
        "//util:config",
        "//proto:config",
        "//external:gflags",
    ],
)

pkg_winzip(
    name = "protones-windows",
    files = [
        ":protones",
    ],
)
