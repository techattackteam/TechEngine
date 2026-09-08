#include <TechEngine/core/jobs/DedicatedThreadContext.hpp>

#include <jobs/DedicatedThreadState.hpp>

#include <mutex>

namespace TechEngine {
    DedicatedThreadContext::DedicatedThreadContext(State& state, const std::stop_token stopToken) : m_state{state}, m_stopToken{stopToken} {
    }

    std::stop_token DedicatedThreadContext::stopToken() const {
        return m_stopToken;
    }

    void DedicatedThreadContext::signalReady() const {
        {
            std::lock_guard const lock{m_state.mutex};
            if (m_state.startupResolved) {
                return;
            }
            m_state.startup = ThreadStartupResult{ThreadStartupStatus::Ready, {}};
            m_state.startupResolved = true;
        }
        m_state.changed.notify_all();
    }
}
