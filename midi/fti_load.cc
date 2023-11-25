#include "midi/fti.h"

int main(int argc, char *argv[]) {
    auto fti = protones::LoadFTI(argv[1]);
    if (fti.ok()) {
        auto inst = fti.value();
        printf("%s\n", inst.DebugString().c_str());
    } else {
        printf("%s\n", fti.status().ToString().c_str());
    }
    return 0;
}
