######################################################################
# gflags
######################################################################
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

patched_http_archive(
    name = "com_github_gflags_gflags",
    patch = "//rules:gflags.patch",
    sha256 = "94ad0467a0de3331de86216cbc05636051be274bf2160f6e86f07345213ba45b",
    strip_prefix = "gflags-77592648e3f3be87d6c7123eb81cbad75f9aef5a",
    urls = ["https://github.com/gflags/gflags/archive/77592648e3f3be87d6c7123eb81cbad75f9aef5a.zip"],
)

bind(
    name = "gflags",
    actual = "@com_github_gflags_gflags//:gflags",
)

######################################################################
# imgui
######################################################################
new_git_repository(
    name = "imgui_git",
    build_file = "//rules:imgui.BUILD",
    remote = "https://github.com/ocornut/imgui.git",
    tag = "v1.74",
)

bind(
    name = "imgui",
    actual = "@imgui_git//:imgui",
)

bind(
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

bind(
    name = "fontawesome",
    actual = "@iconfonts//:fontawesome",
)

######################################################################
# protobuf
######################################################################
git_repository(
    name = "com_google_protobuf",
    remote = "https://github.com/google/protobuf.git",
    tag = "v3.11.1",
)

# rules_cc defines rules for generating C++ code from Protocol Buffers.
http_archive(
    name = "rules_cc",
    sha256 = "35f2fb4ea0b3e61ad64a369de284e4fbbdcdba71836a5555abb5e194cf119509",
    strip_prefix = "rules_cc-624b5d59dfb45672d4239422fa1e3de1822ee110",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_cc/archive/624b5d59dfb45672d4239422fa1e3de1822ee110.tar.gz",
        "https://github.com/bazelbuild/rules_cc/archive/624b5d59dfb45672d4239422fa1e3de1822ee110.tar.gz",
    ],
)

# rules_proto defines abstract rules for building Protocol Buffers.
http_archive(
    name = "rules_proto",
    sha256 = "57001a3b33ec690a175cdf0698243431ef27233017b9bed23f96d44b9c98242f",
    strip_prefix = "rules_proto-9cd4f8f1ede19d81c6d48910429fe96776e567b1",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_proto/archive/9cd4f8f1ede19d81c6d48910429fe96776e567b1.tar.gz",
        "https://github.com/bazelbuild/rules_proto/archive/9cd4f8f1ede19d81c6d48910429fe96776e567b1.tar.gz",
    ],
)

load("@rules_cc//cc:repositories.bzl", "rules_cc_dependencies")

rules_cc_dependencies()

load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")

rules_proto_dependencies()

rules_proto_toolchains()

######################################################################
# pybind11
######################################################################
new_local_repository(
    name = "system_python",
    build_file = "//rules:system_python.BUILD",
    path = "/usr/include/python3.9",
)

new_git_repository(
    name = "pybind11_git",
    build_file = "//rules:pybind11.BUILD",
    remote = "https://github.com/pybind/pybind11.git",
    tag = "v2.4.3",
)

######################################################################
# Abseil
######################################################################
git_repository(
    name = "com_google_absl",
    commit = "ecc56367b8836a552b3716c643da99537c128a13",
    remote = "https://github.com/abseil/abseil-cpp.git",
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

bind(
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
    build_file = "//rules:midifile.BUILD",
    remote = "https://github.com/craigsapp/midifile.git",
    commit = "de6aa0c8f82f9dff29b62dba013a65c9034d633d",
    shallow_since = "1624599610 -0700",
)

######################################################################
# compilers for windows
######################################################################

# Local copy for testing
#local_repository(
#    name = "mxebzl",
#    path = "/home/cfrantz/src/mxebzl",
#)

git_repository(
    name = "mxebzl",
    remote = "https://github.com/cfrantz/mxebzl.git",
    tag = "20191215_RC01",
)

load("@mxebzl//compiler:repository.bzl", "mxe_compiler")

mxe_compiler(
    deps = [
        "SDL2",
        "SDL2-extras",
        "compiler",
        "pthreads",
        "python",
    ],
)
