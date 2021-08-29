#include "midi/fti.h"
#include "util/logging.h"


int main(int argc, char *argv[]) {
    auto fti = protones::LoadFTI(argv[1]);
    if (fti.ok()) {
        auto inst = fti.ValueOrDie();
        printf("%s\n", inst.DebugString().c_str());
    } else {
    }
    return 0;
}
