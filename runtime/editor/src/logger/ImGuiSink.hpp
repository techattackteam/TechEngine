#pragma once

#include <spdlog/sinks/base_sink.h>

namespace TechEngine {
    enum class LogOutput {
        Editor,
        EngineClient,
        EngineServer,
        Client,
        Server,
    };

    struct message {
        LogOutput output;
        spdlog::level::level_enum level;
        std::string time;
        std::string logger;
        std::string levelStr;
        std::string log;
    };

    template<typename Mutex>
    class ImGuiSink : public spdlog::sinks::base_sink<Mutex> {
    private:
        std::vector<message> m_messages;

    public:
        void sink_it_(const spdlog::details::log_msg& msg) override {
            message message;
            if (msg.logger_name == "TechEngineClient") {
                message.output = LogOutput::EngineClient;
            } else if (msg.logger_name == "TechEngineServer") {
                message.output = LogOutput::EngineServer;
            } else if (msg.logger_name == "Client") {
                message.output = LogOutput::Client;
            } else if (msg.logger_name == "Server") {
                message.output = LogOutput::Server;
            } else {
                message.output = LogOutput::Editor;
            }
            message.level = msg.level;
            auto now = std::chrono::time_point_cast<std::chrono::seconds>(
                std::chrono::system_clock::now()
            );
            message.time = std::format("{:%H:%M:%S}", std::chrono::current_zone()->to_local(now));
            message.logger = std::string(msg.logger_name.begin());
            switch (msg.level) {
                case spdlog::level::trace:
                    message.levelStr = "trace";
                    break;
                case spdlog::level::debug:
                    message.levelStr = "debug";
                    break;
                case spdlog::level::info:
                    message.levelStr = "info";
                    break;
                case spdlog::level::warn:
                    message.levelStr = "warn";
                    break;
                case spdlog::level::err:
                    message.levelStr = "error";
                    break;
                case spdlog::level::critical:
                    message.levelStr = "critical";
                    break;
                case spdlog::level::off:
                    message.levelStr = "off";
                    break;
                case spdlog::level::n_levels:
                    message.levelStr = "n_levels";
                    break;
            }
            message.log = std::string(msg.payload.begin());
            m_messages.emplace_back(message);
        }

        void flush_() override {
        }

        void clearMessages() {
            m_messages.clear();
        }

        std::vector<message>& getLogMessages() {
            return m_messages;
        }
    };
}

using ImGuiSink_mt = TechEngine::ImGuiSink<std::mutex>; // Thread-safe sink
