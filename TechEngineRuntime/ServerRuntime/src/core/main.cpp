#include "ServerRuntime.hpp"

int main() {
    auto* app = new TechEngine::ServerRuntime();
    app->init();
    app->run();
    delete app;
    return 0;
}
