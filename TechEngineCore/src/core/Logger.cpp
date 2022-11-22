#include "Logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace TechEngine {
    std::shared_ptr<spdlog::logger> Logger::engineLogger;
    std::shared_ptr<spdlog::logger> Logger::gameLogger;

    void Logger::init() {
        std::vector<spdlog::sink_ptr> logSinks;
        logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("TechEngine.log", true));
        logSinks[0]->set_pattern("%^[%T] %n: %v%$");
        logSinks[1]->set_pattern("[%T] [%l] %n: %v%$");

        engineLogger = std::make_shared<spdlog::logger>("TechEngine", begin(logSinks), end(logSinks));
        spdlog::register_logger(engineLogger);
        engineLogger->set_level(spdlog::level::trace);
        engineLogger->flush_on(spdlog::level::trace);

        gameLogger = std::make_shared<spdlog::logger>("Game", begin(logSinks), end(logSinks));
        spdlog::register_logger(gameLogger);
        gameLogger->set_level(spdlog::level::trace);
        gameLogger->flush_on(spdlog::level::trace);

    }

    std::shared_ptr<spdlog::logger> &Logger::getEngineLogger() {
        return engineLogger;
    }

    std::shared_ptr<spdlog::logger> &Logger::getGameLogger() {
        return gameLogger;
    }
}
