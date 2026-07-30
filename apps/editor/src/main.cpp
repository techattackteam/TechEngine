#include <TechEngine/app/App.hpp>
#include <TechEngine/base/Log.hpp>

int main() {
    TechEngine::registerLogModule("editor");
    TechEngine::initLogging();
    TechEngine::run();
}
