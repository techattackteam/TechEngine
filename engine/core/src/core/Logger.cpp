#include "Logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace TechEngine {
    Logger::Logger(const std::string& name) {
        this->name = name;
    }

    void Logger::init() {
        std::vector<spdlog::sink_ptr> logSinks;
        logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(name + ".log", true));
        logSinks[0]->set_pattern("%^[%T] %n: %v%$");
        logSinks[1]->set_pattern("[%T] [%l] %n: %v%$");

        logger = std::make_shared<spdlog::logger>(name, begin(logSinks), end(logSinks));
        spdlog::register_logger(logger);
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::trace);
    }

    void Logger::shutdown() {
        logger->flush();
    }


    std::shared_ptr<spdlog::logger>& Logger::getLogger() {
        return logger;
    }
}
