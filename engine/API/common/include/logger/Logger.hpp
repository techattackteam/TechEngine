#pragma once

#include "common/include/core/ExportDLL.hpp"

#include <spdlog/logger.h>
#include <spdlog/sinks/dist_sink.h>

namespace TechEngineAPI {
    class API_DLL Logger {
        inline static std::shared_ptr<spdlog::logger> logger;
        inline static bool initialized = false;
        inline static std::string name;
        inline static std::shared_ptr<spdlog::sinks::dist_sink_mt> m_distSinks;

    public:
        static void init(const std::string& name);

        static void shutdown();

        static std::shared_ptr<spdlog::logger> getLogger();

        static spdlog::sinks::dist_sink_mt* getDistSinks();
    };
}

// Core log macros
#define LOGGER_INFO(...)        TechEngineAPI::Logger::getLogger()->info(__VA_ARGS__)
#define LOGGER_TRACE(...)       TechEngineAPI::Logger::getLogger()->trace(__VA_ARGS__)
#define LOGGER_WARN(...)        TechEngineAPI::Logger::getLogger()->warn(__VA_ARGS__)
#define LOGGER_ERROR(...)       TechEngineAPI::Logger::getLogger()->error(__VA_ARGS__)
#define LOGGER_CRITICAL(...) {  TechEngineAPI::Logger::getLogger()->critical(__VA_ARGS__); __debugbreak();}
