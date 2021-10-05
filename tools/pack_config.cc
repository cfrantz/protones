#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "proto/config.pb.h"
#include "util/config.h"

ABSL_FLAG(std::string, config, "", "ROM info config file");
ABSL_FLAG(std::string, symbol, "kConfigText", "Symbol name of config");
ABSL_FLAG(std::string, delimeter, "ZCFGZ", "C++ raw string delimiter");

int main(int argc, char *argv[]) {
    auto args = absl::ParseCommandLine(argc, argv);

    auto* loader = ConfigLoader<proto::Configuration>::Get();
    if (absl::GetFlag(FLAGS_config).empty()) {
        LOG(FATAL, "Need a config file.");
    }

    loader->Load(absl::GetFlag(FLAGS_config));

    proto::Configuration* config = loader->MutableConfig();
    config->mutable_load()->Clear();
    std::string data = config->DebugString();
    
    printf("const char %s[] = R\"%s(%s)%s\";\n",
           absl::GetFlag(FLAGS_symbol).c_str(),
           absl::GetFlag(FLAGS_delimeter).c_str(),
           data.c_str(),
           absl::GetFlag(FLAGS_delimeter).c_str());
    return 0;
}
