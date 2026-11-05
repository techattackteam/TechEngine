#include <edit/ConsoleLog.hpp>

#include <array>
#include <utility>

namespace TechEngine {
    ConsoleLog::ConsoleLog(std::function<void()> wakeMain) : m_wakeMain(std::move(wakeMain)) {
    }

    ConsoleLog::~ConsoleLog() {
        stop();
    }

    std::error_code ConsoleLog::start() {
        m_mainThread = std::this_thread::get_id();
        std::array<LogRecord, LOG_RING_CAPACITY> ring{};
        const std::size_t count = ringSnapshot(ring.data(), ring.size());
        {
            const std::lock_guard lock(m_mutex);
            for (std::size_t i = 0; i < count; i++) {
                push(ring[i]);
            }
        }
        const std::error_code addError = addLogSink(*this);
        if (addError) {
            return addError;
        }
        m_registered = true;
        return {};
    }

    std::error_code ConsoleLog::stop() {
        if (!m_registered) {
            return {};
        }
        const std::error_code removeError = removeLogSink(*this);
        if (removeError) {
            return removeError;
        }
        m_registered = false;
        return {};
    }

    std::size_t ConsoleLog::drain(std::vector<StoredLogRecord>& out) {
        const std::lock_guard lock(m_mutex);
        for (StoredLogRecord& record: m_queue) {
            out.push_back(std::move(record));
        }
        m_queue.clear();
        m_wakePending = false;
        const std::size_t dropped = m_dropped;
        m_dropped = 0;
        return dropped;
    }

    void ConsoleLog::write(const LogRecord& record) noexcept {
        bool wake = false;
        {
            const std::lock_guard lock(m_mutex);
            push(record);
            if (record.level >= Level::Warn && std::this_thread::get_id() != m_mainThread) {
                wake = !m_wakePending.exchange(true);
            }
        }
        if (wake) {
            m_wakeMain();
        }
    }

    void ConsoleLog::push(const LogRecord& record) {
        if (m_queue.size() >= QUEUE_CAPACITY) {
            m_queue.pop_front();
            m_dropped++;
        }
        m_queue.emplace_back(record);
    }
}
