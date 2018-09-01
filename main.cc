#include <cstdio>
#include <string>
#include <gflags/gflags.h>

#include "app.h"
#include "util/config.h"
#include "proto/config.pb.h"
#include "protones_config.h"
#include "pybind11/embed.h"

DEFINE_string(config, "", "ProtoNES config file");

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
    pybind11::scoped_interpreter python{};

    auto* loader = ConfigLoader<proto::Configuration>::Get();
    if (!FLAGS_config.empty()) {
        loader->Load(FLAGS_config);
    } else {
        loader->Parse(kProtonesCfg);
    }

    auto app = std::make_shared<protones::ProtoNES>("ProtoNES");
    protones::ProtoNES::set_python_root(app);
    app->Init();
    if (argc > 1) {
        app->Load(argv[1]);
    }
    app->Run();
    return 0;
}
