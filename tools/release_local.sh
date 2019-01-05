#!/bin/bash

set -e
bazel build \
    --workspace_status_command tools/buildstamp/get_workspace_status \
    -c opt :protones-linux "$@"

sudo tar zxvf bazel-bin/protones-linux.tar.gz --directory=/
