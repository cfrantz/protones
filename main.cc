#include <cstdio>
#include <cstdlib>
#include <string>
#include <gflags/gflags.h>

#include "app.h"
#include "util/config.h"
#include "util/file.h"
#include "util/os.h"
#include "util/logging.h"
#include "proto/config.pb.h"
#include "protones_config.h"
#include "pybind11/embed.h"

DEFINE_string(config, "", "ProtoNES config file");
DEFINE_string(import, "", "Python module to import");

const char kUsage[] =
R"ZZZ(<optional flags> [user-supplied-nes-rom]

Description:
  Nintendo Enterainment System Emulator

Flags:
  --config <filename> Use an alternate config file.
  --hidpi <n>         Set the scaling factor on hidpi displays (try 2.0).
)ZZZ";

namespace py = pybind11;

int main(int argc, char *argv[]) {
    os::SetApplicationName("protones");
    gflags::SetUsageMessage(kUsage);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // Create a python interpreter and add our resource dir to sys.path.
    py::scoped_interpreter python{};
    py::object sys = py::module::import("sys");
    sys.attr("path").attr("append")(os::path::ResourceDir());

    auto* loader = ConfigLoader<proto::Configuration>::Get();
    if (!FLAGS_config.empty()) {
        loader->Load(FLAGS_config);
    } else {
        loader->Parse(kProtonesCfg);
    }

    LOG(INFO, "My datapath is ", os::path::DataPath({}));
    File::MakeDirs(os::path::DataPath());

    auto app = std::make_shared<protones::ProtoNES>("ProtoNES");
    protones::ProtoNES::set_python_root(app);
    app->Init();
    if (!FLAGS_import.empty()) {
        app->Import(FLAGS_import);
    }
    if (argc > 1) {
        app->Load(argv[1]);
    }
    app->Run();
    return 0;
}
