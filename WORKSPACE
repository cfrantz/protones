######################################################################
# gflags
######################################################################
load("//rules:patched_http_archive.bzl",
     "patched_http_archive",
     "new_patched_http_archive")

patched_http_archive(
    name = "com_github_gflags_gflags",
    urls = ["https://github.com/gflags/gflags/archive/77592648e3f3be87d6c7123eb81cbad75f9aef5a.zip"],
    strip_prefix = "gflags-77592648e3f3be87d6c7123eb81cbad75f9aef5a",
    sha256 = "94ad0467a0de3331de86216cbc05636051be274bf2160f6e86f07345213ba45b",
    patch = "//rules:gflags.patch",
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
    tag = "v1.60",
    remote = "https://github.com/ocornut/imgui.git",
    build_file = "//rules:imgui.BUILD",
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
new_patched_http_archive(
    name = "com_github_bimpy",
    urls = ["https://github.com/podgorskiy/bimpy/archive/54fa78cec38b48770a486ac41404e08465c789e0.zip"],
    strip_prefix = "bimpy-54fa78cec38b48770a486ac41404e08465c789e0",
    sha256 = "f691eaf43ece6b5e7b85b0419c12753ad2512a860a5abe88f70a3ebdd3871b20",
    patch = "//rules:bimpy.patch",
    build_file = "//rules:bimpy.BUILD",
)

######################################################################
# IconFontCppHeaders
######################################################################
new_git_repository(
	name = "iconfonts",
	remote = "https://github.com/juliettef/IconFontCppHeaders.git",
    commit = "fda5f470b767f7b413e4a3995fa8cfe47f78b586",
    build_file = "//rules:iconfonts.BUILD",
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
	tag = "v3.5.0"
)

######################################################################
# pybind11
######################################################################
new_git_repository(
	name = "pybind11_git",
	remote = "https://github.com/pybind/pybind11.git",
	tag = "v2.2.3",
    build_file = "//rules:pybind11.BUILD",
)

######################################################################
# Abseil
######################################################################
git_repository(
    name = "com_google_absl",
    remote = "https://github.com/abseil/abseil-cpp.git",
    commit = "ecc56367b8836a552b3716c643da99537c128a13",
)

######################################################################
# native file dialog
######################################################################
new_git_repository(
	name = "nativefiledialog_git",
	remote = "https://github.com/mlabbe/nativefiledialog.git",
    commit = "5cfe5002eb0fac1e49777a17dec70134147931e2",
    build_file = "//rules:nfd.BUILD",
)

bind(
    name = "nfd",
    actual = "@nativefiledialog_git//:nfd",
)


######################################################################
# compilers for windows
######################################################################
git_repository(
    name = "mxebzl",
    remote = "https://github.com/cfrantz/mxebzl.git",
	tag = "20181103_RC02",
)

load("@mxebzl//tools:repository.bzl", "mxe_compilers")
mxe_compilers(
    deps = [
        "compiler",
        "SDL2",
        "SDL2-extras",
        "pthreads",
        "python",
    ],
)
