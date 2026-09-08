#pragma once

#include <stop_token>

namespace TechEngine {
    class DedicatedThreadContext {
    public:
        DedicatedThreadContext(const DedicatedThreadContext&) = delete;

        DedicatedThreadContext& operator=(const DedicatedThreadContext&) = delete;

        std::stop_token stopToken() const;

        void signalReady() const;

    private:
        friend class DedicatedThread;
        friend class JobSystem;

        struct State;

        DedicatedThreadContext(State& state, std::stop_token stopToken);

        State& m_state;
        std::stop_token m_stopToken;
    };
}
