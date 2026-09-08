#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/jobs/ThreadRegistration.hpp>

#include <mutex>
#include <utility>

namespace TechEngine {
    ThreadRegistration::ThreadRegistration(JobSystem& jobs, std::string name, const ThreadRole role) : m_jobs{jobs}, m_threadId{std::this_thread::get_id()} {
        TE_PROFILER_THREAD_NAME(name.c_str());

        bool inserted = false;
        {
            std::lock_guard const lock{m_jobs.m_registryMutex};
            inserted = m_jobs.m_registeredThreads.emplace(m_threadId, ThreadInfo{m_threadId, std::move(name), role}).second;
        }
        TE_CHECK(inserted, "This thread is already registered with the job system");
    }

    ThreadRegistration::~ThreadRegistration() {
        TE_CHECK(m_threadId == std::this_thread::get_id(), "A thread registration must be released on its executing thread");
        std::lock_guard const lock{m_jobs.m_registryMutex};
        m_jobs.m_registeredThreads.erase(m_threadId);
    }
}
