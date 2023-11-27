#include <stdio.h>

#include "absl/log/log.h"
#include "util/sound/file.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        LOG(ERROR) << argv[0] << " [inputwav] [outfile]";
        return 1;
    }

    auto f = sound::File::Load(argv[1]);
    if (!f.ok()) {
        LOG(ERROR) << "Failed to read input: " << f.status();
        return 1;
    }
    auto sts = (*f)->Save(argv[2]);
    if (!sts.ok()) {
        LOG(ERROR) << "Failed to save: " << sts;
    }
    return 0;
}
