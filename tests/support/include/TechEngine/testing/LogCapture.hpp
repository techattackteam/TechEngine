#pragma once

#include <TechEngine/base/diagnostics/Log.hpp>

#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace TechEngineTests {
    struct CapturedLog {
        std::chrono::system_clock::time_point time;
        std::uint64_t tick = 0;
        TechEngine::Level level = TechEngine::Level::Info;
        TechEngine::LogModule moduleTag{};
        TechEngine::LogChannel channel{};
        std::string message;
        std::string file;
        std::string function;
        std::uint32_t line = 0;
    };

    // Diagnostics state is process-global and Catch2 shares one process, so the guard restores
    // the min level it set before the next case runs.
    class LogCaptureGuard final : public TechEngine::LogSink {
    public:
        explicit LogCaptureGuard(const TechEngine::Level minLevel = TechEngine::Level::Trace) : m_previousLevel(TechEngine::minLevel()) {
            TechEngine::setMinLevel(minLevel);
            TechEngine::addLogSink(*this);
        }

        ~LogCaptureGuard() override {
            TechEngine::removeLogSink(*this);
            TechEngine::setMinLevel(m_previousLevel);
        }

        LogCaptureGuard(const LogCaptureGuard&) = delete;

        LogCaptureGuard& operator=(const LogCaptureGuard&) = delete;

        void write(const TechEngine::LogRecord& record) noexcept override {
            const std::lock_guard<std::mutex> lock{m_mutex};
            m_records.push_back(CapturedLog{
                .time = record.time,
                .tick = record.tick,
                .level = record.level,
                .moduleTag = record.moduleTag,
                .channel = record.channel,
                .message = std::string{record.message},
                .file = std::string{record.file},
                .function = std::string{record.function},
                .line = record.line,
            });
        }

        std::vector<CapturedLog> getRecords() const {
            const std::lock_guard<std::mutex> lock{m_mutex};
            return m_records;
        }

        std::vector<std::string> getMessages() const {
            std::vector<std::string> messages;
            for (const CapturedLog& record: getRecords()) {
                messages.push_back(record.message);
            }
            return messages;
        }

        bool contains(const std::string_view text) const {
            for (const CapturedLog& record: getRecords()) {
                if (record.message.find(text) != std::string::npos) {
                    return true;
                }
            }
            return false;
        }

        void clear() {
            const std::lock_guard<std::mutex> lock{m_mutex};
            m_records.clear();
        }

    private:
        mutable std::mutex m_mutex;
        std::vector<CapturedLog> m_records;
        TechEngine::Level m_previousLevel;
    };
}
