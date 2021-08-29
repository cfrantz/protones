package(default_visibility = ["//visibility:public"])

load("@bazel_tools//tools/build_defs/pkg:pkg.bzl", "pkg_tar")
load("@mxebzl//tools:rules.bzl", "pkg_winzip")

config_setting(
    name = "windows",
    values = {
        "crosstool_top": "@mxebzl//compiler:win64",
    },
)

genrule(
    name = "make_version",
    outs = ["version.h"],
    cmd = """sed -e 's/ / "/g' -e 's/$$/"\\n/g' -e "s/^/#define /g" bazel-out/volatile-status.txt > $@""",
    stamp = 1,
)

cc_library(
    name = "app",
    srcs = [
        "app.cc",
    ],
    hdrs = [
        "app.h",
        "version.h",
    ],
    linkopts = [
        "-lSDL2_image",
        "-lSDL2_mixer",
        "-lSDL2_gfx",
        "-lSDL2",
    ],
    deps = [
        "//imwidget:base",
        "//imwidget:apu_debug",
        "//imwidget:controller_debug",
        "//imwidget:mem_debug",
        "//imwidget:midi_setup",
        "//imwidget:ppu_debug",
        "//imwidget:python_console",
        "//imwidget:error_dialog",
        "//nes",
        "//python:protones",
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
        "@pybind11_git//:pybind11",
    ],
)

genrule(
    name = "make_protones_config",
    srcs = [
        "protones.textpb",
        "//content:controls",
    ],
    outs = ["protones_config.h"],
    cmd = "$(location //tools:pack_config) --config $(location protones.textpb)" +
          " --symbol kProtonesCfg > $(@)",
    tools = ["//tools:pack_config"],
)

cc_binary(
    name = "protones",
    srcs = [
        "main.cc",
        "protones_config.h",
    ],
    data = ["//content"] + select({
        ":windows": ["@mxebzl//runtime/python37"],
        "//conditions:default": [],
    }),
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
            "-lmingw32",
            "-lSDL2main",
        ],
        "//conditions:default": [
            "-lpthread",
            "-lm",
            "-lGL",
        ],
    }),
    deps = [
        ":app",
        "//external:gflags",
        "//proto:config",
        "//util:config",
        "//util:file",
        "//util:os",
        "@com_github_bimpy//:bimpy",
        "@pybind11_git//:pybind11",
    ],
)

pkg_winzip(
    name = "protones-windows",
    files = [
        ":protones",
        "//content",
    ],
    skip_dlls = [
        # Supplied by the python runtime
        "python37.dll",
    ],
    zips = [
        "@mxebzl//runtime/python37",
    ],
)

pkg_tar(
    name = "protones-bin",
    srcs = [
        ":protones",
    ],
    mode = "0755",
    package_dir = "/usr/local/bin",
)

pkg_tar(
    name = "protones-share",
    srcs = [
        "//content",
    ],
    mode = "0644",
    package_dir = "/usr/local/share/protones",
    # BUG: https://github.com/bazelbuild/bazel/issues/2176
    strip_prefix = ".",
)

pkg_tar(
    name = "protones-linux",
    extension = "tar.gz",
    deps = [
        ":protones-bin",
        ":protones-share",
    ],
)
