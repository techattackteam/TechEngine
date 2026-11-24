#pragma once

#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/StoredLogRecord.hpp>

#include <atomic>
#include <cstddef>
#include <deque>
#include <functional>
#include <mutex>
#include <system_error>
#include <thread>
#include <vector>

namespace TechEngine {
    class ConsoleLog final : public LogSink {
    private:
        std::function<void()> m_wakeMain;
        std::thread::id m_mainThread;
        std::mutex m_mutex;
        std::deque<StoredLogRecord> m_queue;
        std::size_t m_dropped = 0;
        std::atomic<bool> m_wakePending = false;
        // Main thread only.
        bool m_registered = false;

    public:
        static constexpr std::size_t QUEUE_CAPACITY = 1024;

        explicit ConsoleLog(std::function<void()> wakeMain);

        ~ConsoleLog() override;

        ConsoleLog(const ConsoleLog&) = delete;

        ConsoleLog& operator=(const ConsoleLog&) = delete;

        std::error_code start();

        std::error_code stop();

        std::size_t drain(std::vector<StoredLogRecord>& out);

        void write(const LogRecord& record) noexcept override;

    private:
        void push(StoredLogRecord record);
    };
}
