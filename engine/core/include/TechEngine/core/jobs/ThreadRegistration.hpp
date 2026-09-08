#pragma once

#include <string>
#include <thread>

namespace TechEngine {
    class JobSystem;
    enum class ThreadRole;

    class ThreadRegistration {
    public:
        ~ThreadRegistration();

        ThreadRegistration(const ThreadRegistration&) = delete;

        ThreadRegistration& operator=(const ThreadRegistration&) = delete;

        ThreadRegistration(ThreadRegistration&&) = delete;

        ThreadRegistration& operator=(ThreadRegistration&&) = delete;

    private:
        friend class JobSystem;

        ThreadRegistration(JobSystem& jobs, std::string name, ThreadRole role);

        JobSystem& m_jobs;
        std::thread::id m_threadId;
    };
}
