#include <TechEngine/app/App.hpp>

#include "TechEngine/base/Log.hpp"


int main() {
    TechEngine::initLogging();
    int x = 10;
    int y = -10;
    TE_LOGGER_INFO("Starting TechEngine Editor: {1}, {0}", x, y);
    return TechEngine::run();
}
