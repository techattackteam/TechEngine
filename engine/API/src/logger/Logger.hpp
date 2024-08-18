#pragma once


#include <string>
#include <vector>

#include "core/ExportDLL.hpp"

namespace TechEngine {
    class Logger;
}

namespace TechEngineAPI {
    class API_DLL Logger {
        inline static Logger* logger = nullptr;
        TechEngine::Logger* loggerCore;

    public:
        Logger(TechEngine::Logger* loggerCore);

        static Logger* getLogger();

    private:
    };
}

#define LOGGER_INFO(...) TechEngine::Logger::getLogger()->info(__VA_ARGS__)
