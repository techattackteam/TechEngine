#include "Logger.hpp"
#include "core/Logger.hpp"

namespace TechEngineAPI {
    Logger::Logger(TechEngine::Logger* loggerCore) : loggerCore(loggerCore) {
        logger = this;
    }
}
