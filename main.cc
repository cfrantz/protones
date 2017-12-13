#include <cstdio>
#include <string>
#include <gflags/gflags.h>

#include "app.h"
#include "util/config.h"

const char kUsage[] =
R"ZZZ(<optional flags> [user-supplied-nes-rom]

Description:
  Nintendo Enterainment System Emulator

Flags:
  --config <filename> Use an alternate config file.
  --hidpi <n>         Set the scaling factor on hidpi displays (try 2.0).
)ZZZ";

int main(int argc, char *argv[]) {
    gflags::SetUsageMessage(kUsage);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    protones::ProtoNES app("ProtoNES");
    app.Init();
    if (argc > 1) {
        app.Load(argv[1]);
    }
    app.Run();
    return 0;
}
