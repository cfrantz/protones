#include "app.h"

int main(int argc, char* argv[]) {
    auto app = std::make_unique<protones::ProtoNES>();
    app->Init();
    if (argc > 1) {
        app->Load(argv[1]);
    }
    app->Run();
    return 0;
}
