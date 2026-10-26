#include <TechEngine/app/CommandQueue.hpp>
#include <TechEngine/app/SimulationError.hpp>
#include <TechEngine/base/diagnostics/Assert.hpp>

#include <utility>

namespace TechEngine {
    CommandQueue::CommandQueue(const std::size_t capacity) : m_capacity(capacity) {
        TE_CHECK(capacity > 0, "Command capacity must be positive");
        m_pending.reserve(m_capacity);
    }

    std::error_code CommandQueue::submit(std::unique_ptr<Command> command) {
        TE_CHECK(command != nullptr, "A submitted command must not be null");
        const std::lock_guard lock{m_mutex};
        if (m_closed) {
            return SimulationError::CommandQueueClosed;
        }
        if (m_pending.size() >= m_capacity) {
            return SimulationError::CommandQueueFull;
        }
        m_pending.push_back(std::move(command));
        return {};
    }

    void CommandQueue::consume(std::vector<std::unique_ptr<Command>>& batch) {
        const std::lock_guard lock{m_mutex};
        batch.reserve(m_pending.size());
        for (auto& command: m_pending) {
            batch.push_back(std::move(command));
        }
        m_pending.clear();
    }

    void CommandQueue::close() {
        const std::lock_guard lock{m_mutex};
        m_closed = true;
    }

    void CommandQueue::open() {
        const std::lock_guard lock{m_mutex};
        m_closed = false;
    }

    std::size_t CommandQueue::capacity() const {
        return m_capacity;
    }
}
