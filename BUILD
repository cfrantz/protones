package(default_visibility=["//visibility:public"])

load("@bazel_tools//tools/build_defs/pkg:pkg.bzl", "pkg_tar")
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
        "//imwidget:python_console",
        "//imwidget:error_dialog",
        "//nes:nes",
        "//python:protones",
        "//util:browser",
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
        "@pybind11_git//:pybind11",
        "@com_github_bimpy//:bimpy",
    ],
    data = ["//content"] + select({
        ":windows": ["@mxebzl//runtime/python37"],
        "//conditions:default": [],
    })
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
    package_dir = "/usr/local/bin",
    mode = "0755",
)

pkg_tar(
    name = "protones-share",
    srcs = [
        "//content",
    ],
    # BUG: https://github.com/bazelbuild/bazel/issues/2176
    strip_prefix = ".",
    package_dir = "/usr/local/share/protones",
    mode = "0644",
)

pkg_tar(
    name = "protones-linux",
    extension = "tar.gz",
    deps = [
        ":protones-bin",
        ":protones-share",
    ],
)
