#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/jobs/DedicatedThread.hpp>

#include <jobs/DedicatedThreadState.hpp>

#include <mutex>
#include <utility>

namespace TechEngine {
    DedicatedThread::DedicatedThread() = default;

    DedicatedThread::~DedicatedThread() = default;

    DedicatedThread::DedicatedThread(DedicatedThread&& other) noexcept = default;

    DedicatedThread& DedicatedThread::operator=(DedicatedThread&& other) noexcept {
        if (this != &other) {
            m_thread = std::move(other.m_thread);
            m_state = std::move(other.m_state);
        }
        return *this;
    }

    void DedicatedThread::requestStop() {
        m_thread.request_stop();
    }

    void DedicatedThread::join() {
        if (!m_thread.joinable()) {
            return;
        }
        if (!TE_ENSURE(m_thread.get_id() != std::this_thread::get_id(), "A dedicated thread cannot join itself")) {
            return;
        }
        m_thread.join();
    }

    bool DedicatedThread::joinable() const {
        return m_thread.joinable();
    }

    ThreadStartupResult DedicatedThread::waitUntilReady() const {
        if (!m_state) {
            return {};
        }
        std::unique_lock lock{m_state->mutex};
        m_state->changed.wait(lock, [this] {
            return m_state->startupResolved;
        });
        return m_state->startup;
    }

    ThreadCompletionResult DedicatedThread::completion() const {
        if (!m_state) {
            return ThreadCompletionResult{ThreadCompletionStatus::Completed, {}};
        }
        std::lock_guard const lock{m_state->mutex};
        return m_state->completion;
    }

    ThreadCompletionResult DedicatedThread::waitUntilComplete() const {
        if (!m_state) {
            return ThreadCompletionResult{ThreadCompletionStatus::Completed, {}};
        }
        std::unique_lock lock{m_state->mutex};
        m_state->changed.wait(lock, [this] {
            return m_state->completion.status != ThreadCompletionStatus::Running;
        });
        return m_state->completion;
    }
}
