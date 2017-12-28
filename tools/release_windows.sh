#!/bin/bash

#    --workspace_status_command tools/buildstamp/get_workspace_status \
set -e
bazel build \
    --crosstool_top=@mxebzl//tools/windows:toolchain --cpu=win64 \
    -c opt :protones-windows

VERSION=$(grep BUILD_GIT_VERSION bazel-out/volatile-status.txt | cut -f2 -d' ')

cp bazel-bin/protones-windows.zip protones-windows-${VERSION}.zip
