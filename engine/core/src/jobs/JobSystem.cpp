#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>

#include <format>
#include <string>
#include <utility>

namespace TechEngine {
    JobSystem::JobSystem(const std::size_t workerCount) {
        std::size_t count = workerCount;
        if (count == 0) {
            TE_CHECK(false, "A job system needs at least one worker; starting one");
            count = 1;
        }

        m_running = true;
        m_workers.reserve(count);
        for (std::size_t i = 0; i < count; i++) {
            m_workers.emplace_back([this, i] {
                workerMain(i);
            });
        }
    }

    JobSystem::~JobSystem() {
        shutdown();
    }

    BatchId JobSystem::submit(const std::span<Task> tasks) {
        if (tasks.empty()) {
            return {};
        }

        std::uint64_t id = 0;
        {
            std::lock_guard const lock{m_mutex};
            if (!m_running) {
                id = 0;
            } else {
                id = m_nextBatchId++;
                m_pendingBatches.emplace(id, tasks.size());
                for (Task& task: tasks) {
                    m_queue.push_back(QueuedTask{std::move(task), id});
                }
            }
        }

        if (id == 0) {
            TE_CHECK(false, "Work submitted after the job system shut down");
            return {};
        }

        m_workAvailable.notify_all();
        return BatchId{id};
    }

    void JobSystem::wait(const BatchId batch) {
        if (!batch.valid()) {
            return;
        }

        TE_PROFILER_SCOPE("JobSystem.Wait");

        std::unique_lock lock{m_mutex};
        m_batchComplete.wait(lock, [this, batch] {
            return !m_pendingBatches.contains(batch.value());
        });
    }

    void JobSystem::shutdown() {
        {
            std::lock_guard const lock{m_mutex};
            if (!m_running) {
                return;
            }
            m_running = false;
        }

        m_workAvailable.notify_all();

        for (std::thread& worker: m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    std::size_t JobSystem::workerCount() const {
        return m_workers.size();
    }

    void JobSystem::workerMain(const std::size_t workerIndex) {
        const std::string threadName = std::format("TEWorker{}", workerIndex);
        TE_PROFILER_THREAD_NAME(threadName.c_str());

        while (true) {
            QueuedTask queued;
            {
                std::unique_lock lock{m_mutex};
                m_workAvailable.wait(lock, [this] {
                    return !m_running || !m_queue.empty();
                });

                if (m_queue.empty()) {
                    return;
                }

                queued = std::move(m_queue.front());
                m_queue.pop_front();
            }

            {
                TE_PROFILER_SCOPE("JobSystem.Task");
                queued.task();
            }

            bool completed = false;
            {
                std::lock_guard const lock{m_mutex};
                const auto it = m_pendingBatches.find(queued.batchId);
                if (it != m_pendingBatches.end()) {
                    it->second--;
                    if (it->second == 0) {
                        m_pendingBatches.erase(it);
                        completed = true;
                    }
                }
            }

            if (completed) {
                m_batchComplete.notify_all();
            }
        }
    }
}
