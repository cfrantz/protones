# Copyright lowRISC contributors.
# Licensed under the Apache License, Version 2.0, see LICENSE for details.
# SPDX-License-Identifier: Apache-2.0

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

def midi_repos():
    git_repository(
        name = "com_github_rtmidi",
        build_file = "//third_party/midi:BUILD.rtmidi.bazel",
        remote = "https://github.com/thestk/rtmidi.git",
        tag = "2.1.1",
    )

    git_repository(
        name = "com_github_midifile",
        build_file = "//third_party/midi:BUILD.midifile.bazel",
        remote = "https://github.com/craigsapp/midifile.git",
        commit = "de6aa0c8f82f9dff29b62dba013a65c9034d633d",
        shallow_since = "1624599610 -0700",
    )
