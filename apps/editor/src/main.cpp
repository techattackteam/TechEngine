#include <TechEngine/app/App.hpp>

#include "TechEngine/base/Log.hpp"

int main() {
    TechEngine::initLogging();
    int x = 10;
    int y = -10;
    TE_LOGGER_INFO("Starting TechEngine Editor: {1}, {0}", x, y);
    TE_LOGGER_WARN("Starting TechEngine Editor: {1}, {0}", x, y);
    TE_LOGGER_ERROR("Starting TechEngine Editor: {1}, {0}", x, y);
    TE_LOGGER_CRITICAL("Starting TechEngine Editor: {1}, {0}", x, y);
    TE_LOGGER_DEBUG("Starting TechEngine Editor: {1}, {0}", x, y);
    TE_LOGGER_TRACE("Starting TechEngine Editor: {1}, {0}", x, y);
    return TechEngine::run();
}
