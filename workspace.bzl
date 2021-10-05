load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load(
    "//rules:patched_http_archive.bzl",
    "new_patched_http_archive",
    "patched_http_archive",
)
load(
    "@bazel_tools//tools/build_defs/repo:git.bzl",
    "git_repository",
    "new_git_repository",
)

def protones_deps():
    ######################################################################
    # imgui
    ######################################################################
    new_git_repository(
        name = "imgui_git",
        build_file = "//rules:imgui.BUILD",
        remote = "https://github.com/ocornut/imgui.git",
        tag = "v1.74",
    )

    native.bind(
        name = "imgui",
        actual = "@imgui_git//:imgui",
    )

    native.bind(
        name = "imgui_sdl_opengl",
        actual = "@imgui_git//:imgui_sdl_opengl",
    )

    ######################################################################
    # bimpy (python bindings for imgui)
    ######################################################################
    new_git_repository(
        name = "com_github_bimpy",
        remote = "https://github.com/podgorskiy/bimpy.git",
        build_file = "//rules:bimpy.BUILD",
        tag = "v0.1.0",
        patches = ["//rules:bimpy.patch"],
        patch_args = ["-p1"],
    )

    ######################################################################
    # IconFontCppHeaders
    ######################################################################
    new_git_repository(
        name = "iconfonts",
        build_file = "//rules:iconfonts.BUILD",
        commit = "fda5f470b767f7b413e4a3995fa8cfe47f78b586",
        remote = "https://github.com/juliettef/IconFontCppHeaders.git",
    )

    native.bind(
        name = "fontawesome",
        actual = "@iconfonts//:fontawesome",
    )

    ######################################################################
    # Abseil
    ######################################################################
    git_repository(
        name = "com_google_absl",
        #commit = "ecc56367b8836a552b3716c643da99537c128a13",
        remote = "https://github.com/abseil/abseil-cpp.git",
        tag = "20210324.2",
    )

    ######################################################################
    # protobuf
    ######################################################################
    git_repository(
        name = "com_google_protobuf",
        remote = "https://github.com/google/protobuf.git",
        tag = "v3.18.0",
    )

    ######################################################################
    # pybind11
    ######################################################################
    new_git_repository(
        name = "pybind11_git",
        build_file = "//rules:pybind11.BUILD",
        remote = "https://github.com/pybind/pybind11.git",
        tag = "v2.4.3",
    )

    ######################################################################
    # native file dialog
    ######################################################################
    new_git_repository(
        name = "nativefiledialog_git",
        build_file = "//rules:nfd.BUILD",
        commit = "5cfe5002eb0fac1e49777a17dec70134147931e2",
        remote = "https://github.com/mlabbe/nativefiledialog.git",
    )

    native.bind(
        name = "nfd",
        actual = "@nativefiledialog_git//:nfd",
    )

    ######################################################################
    # rtmidi
    ######################################################################
    new_git_repository(
        name = "rtmidi_git",
        build_file = "//rules:rtmidi.BUILD",
        remote = "https://github.com/thestk/rtmidi.git",
        tag = "2.1.1",
    )

    ######################################################################
    # midifile
    ######################################################################
    new_git_repository(
        name = "midifile_git",
        build_file = "@protones//rules:midifile.BUILD",
        remote = "https://github.com/craigsapp/midifile.git",
        commit = "de6aa0c8f82f9dff29b62dba013a65c9034d633d",
        shallow_since = "1624599610 -0700",
    )
