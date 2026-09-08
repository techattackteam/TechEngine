#pragma once

#include <TechEngine/core/jobs/DedicatedThreadContext.hpp>

#include <exception>
#include <memory>
#include <string>
#include <thread>

namespace TechEngine {
    enum class ThreadRole { Host, Dedicated, PoolWorker };

    struct ThreadInfo {
        std::thread::id id;
        std::string name;
        ThreadRole role = ThreadRole::Dedicated;
    };

    enum class ThreadStartupStatus { Ready, Failed };

    struct ThreadStartupResult {
        ThreadStartupStatus status = ThreadStartupStatus::Failed;
        std::exception_ptr failure;
    };

    enum class ThreadCompletionStatus { Running, Completed, Failed };

    struct ThreadCompletionResult {
        ThreadCompletionStatus status = ThreadCompletionStatus::Running;
        std::exception_ptr failure;
    };

    class DedicatedThread {
    public:
        DedicatedThread();

        ~DedicatedThread();

        DedicatedThread(const DedicatedThread&) = delete;

        DedicatedThread& operator=(const DedicatedThread&) = delete;

        DedicatedThread(DedicatedThread&& other) noexcept;

        DedicatedThread& operator=(DedicatedThread&& other) noexcept;

        void requestStop();

        void join();

        bool joinable() const;

        ThreadStartupResult waitUntilReady() const;

        ThreadCompletionResult completion() const;

        ThreadCompletionResult waitUntilComplete() const;

    private:
        friend class JobSystem;

        std::unique_ptr<DedicatedThreadContext::State> m_state;
        std::jthread m_thread;
    };
}
