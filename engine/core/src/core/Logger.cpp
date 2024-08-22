#include "Logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/dist_sink.h>

namespace TechEngine {
    Logger::Logger(const std::string& name) {
        this->name = name;
    }

    void Logger::init() {
        m_distSinks = std::make_shared<spdlog::sinks::dist_sink_mt>();
        std::vector<spdlog::sink_ptr> logSinks;
        logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(name + ".log", true));
        logSinks.emplace_back(m_distSinks);
        logSinks[0]->set_pattern("%^[%T] %n: %v%$");
        logSinks[1]->set_pattern("[%T] [%l] %n: %v%$");
        logSinks[2]->set_pattern("[%T] [%l] %n: %v%$");

        logger = std::make_shared<spdlog::logger>(name, begin(logSinks), end(logSinks));
        spdlog::register_logger(logger);
        logger->set_level(spdlog::level::trace);
        logger->flush_on(spdlog::level::trace);
    }

    void Logger::shutdown() {
        logger->flush();
    }

    void Logger::addLogSink(const std::shared_ptr<spdlog::sinks::sink>& sink) {
        m_distSinks->add_sink(sink);
    }


    std::shared_ptr<spdlog::logger>& Logger::getLogger() {
        return logger;
    }
}
