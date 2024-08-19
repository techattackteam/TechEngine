#pragma once


#include "core/ExportDLL.hpp"
#include <spdlog/logger.h>

namespace TechEngineAPI {
    class API_DLL Logger {
        inline static std::shared_ptr<spdlog::logger> logger;
        inline static bool initialized = false;
        inline static std::string name;

    public:
        static void init(const std::string& name);

        static void shutdown();

        static std::shared_ptr<spdlog::logger>& getLogger();
    };
}

// Core log macros
#define LOGGER_INFO(...)        TechEngineAPI::Logger::getLogger()->info(__VA_ARGS__)
#define LOGGER_TRACE(...)       TechEngineAPI::Logger::getLogger()->trace(__VA_ARGS__)
#define LOGGER_WARN(...)        TechEngineAPI::Logger::getLogger()->warn(__VA_ARGS__)
#define LOGGER_ERROR(...)       TechEngineAPI::Logger::getLogger()->error(__VA_ARGS__)
#define LOGGER_CRITICAL(...) {  TechEngineAPI::Logger::getLogger()->critical(__VA_ARGS__); __debugbreak();}
