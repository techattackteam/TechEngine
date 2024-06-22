#include "Editor.hpp"

int main() {
    auto* app = new TechEngine::Editor();
    app->run();
    delete app;
    return 0;
}
