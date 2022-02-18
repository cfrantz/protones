#include <cstdio>
#include <cstdlib>
#include <string>
#include <SDL2/SDL.h>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "app.h"
#include "util/config.h"
#include "util/file.h"
#include "util/os.h"
#include "util/logging.h"
#include "proto/config.pb.h"
#include "protones_config.h"
#include "pybind11/embed.h"

ABSL_FLAG(std::string, config, "", "ProtoNES config file");
ABSL_FLAG(std::string, import, "", "Python module to import");
ABSL_FLAG(std::vector<std::string>, extra, {}, "Extra args made available to python");

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
    absl::SetProgramUsageMessage(kUsage);
    auto args = absl::ParseCommandLine(argc, argv);

    // Create a python interpreter and add our resource dir to sys.path.
    py::scoped_interpreter python{};
    py::object sys = py::module::import("sys");
    sys.attr("path").attr("append")(os::path::ResourceDir());

    auto* loader = ConfigLoader<proto::Configuration>::Get();
    if (!absl::GetFlag(FLAGS_config).empty()) {
        loader->Load(absl::GetFlag(FLAGS_config));
    } else {
        loader->Parse(kProtonesCfg);
    }

    LOG(INFO, "My datapath is ", os::path::DataPath({}));
    File::MakeDirs(os::path::DataPath()).IgnoreError();

    auto app = std::make_shared<protones::ProtoNES>("ProtoNES");
    protones::ProtoNES::set_python_root(app);
    app->Init();
    if (!absl::GetFlag(FLAGS_import).empty()) {
        app->Import(absl::GetFlag(FLAGS_import));
    }
    if (args.size() > 1) {
        printf("Loading %s\n", args[1]);
        app->Load(args[1]);
    }
    app->Run();
    return 0;
}
