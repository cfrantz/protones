#include <cstdio>
#include <string>
#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"

#include "ips/ips.h"
#include "util/file.h"

ABSL_FLAG(bool, create, false, "Create an IPS patch");
ABSL_FLAG(bool, apply, false, "Apply an IPS patch");

const char kUsage[] =
R"ZZZ(<flags> [files...]

Description:
  A Simple IPS patch utility.

Usage:
  ipspatch -create [original] [modified] [patch-output-file]
  ipspatch -apply [origina] [patch-file] [modified-output-file]
)ZZZ";

int main(int argc, char *argv[]) {
    absl::SetProgramUsageMessage(kUsage);
    auto args = absl::ParseCommandLine(argc, argv);
    std::string original, modified, patch;

    if (absl::GetFlag(FLAGS_create) && args.size() == 4) {
        File::GetContents(args[1], &original);
        File::GetContents(args[2], &modified);
        patch = ips::CreatePatch(original, modified);
        File::SetContents(args[3], patch);
        printf("Wrote patch to %s\n", argv[3]);
    } else if (absl::GetFlag(FLAGS_apply) && args.size() == 4) {
        File::GetContents(args[1], &original);
        File::GetContents(args[2], &patch);
        auto mod = ips::ApplyPatch(original, patch);
        if (mod.ok()) {
            File::SetContents(args[3], mod.value());
            printf("Applied patch and wrote new file %s\n", argv[3]);
        } else {
            printf("Error: %s\n", mod.status().ToString().c_str());
        }
    } else {
        printf("%s %s\n", argv[0], kUsage);
    }
    return 0;
}
