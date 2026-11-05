#pragma once

#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/StoredLogRecord.hpp>

#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace TechEngineTests {
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
            m_records.emplace_back(record);
        }

        std::vector<TechEngine::StoredLogRecord> getRecords() const {
            const std::lock_guard<std::mutex> lock{m_mutex};
            return m_records;
        }

        std::vector<std::string> getMessages() const {
            std::vector<std::string> messages;
            for (const TechEngine::StoredLogRecord& record: getRecords()) {
                messages.push_back(record.message);
            }
            return messages;
        }

        bool contains(const std::string_view text) const {
            for (const TechEngine::StoredLogRecord& record: getRecords()) {
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
        std::vector<TechEngine::StoredLogRecord> m_records;
        TechEngine::Level m_previousLevel;
    };
}
