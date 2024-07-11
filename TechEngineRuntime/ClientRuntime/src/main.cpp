#include "TechEngineRuntime.hpp"

int main() {
    auto* app = new TechEngine::TechEngineRuntime();
    app->init();
    app->run();
    delete app;
    return 0;
}
