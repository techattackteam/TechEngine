#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>

#include <exception>
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

        try {
            for (std::size_t i = 0; i < count; i++) {
                m_workers.emplace_back([this, i] {
                    workerMain(i);
                });
            }
        } catch (...) {
            shutdown();
            throw;
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

        if (isWorkerThread()) {
            TE_CHECK(false, "wait() called from a pool worker; barriers never run on workers");
            return;
        }

        TE_PROFILER_SCOPE("JobSystem.Wait");

        std::unique_lock lock{m_mutex};
        m_batchComplete.wait(lock, [this, batch] {
            return !m_pendingBatches.contains(batch.value());
        });
    }

    void JobSystem::shutdown() {
        std::lock_guard const shutdownLock{m_shutdownMutex};

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

    bool JobSystem::isWorkerThread() const {
        const std::thread::id self = std::this_thread::get_id();
        for (const std::thread& worker: m_workers) {
            if (worker.get_id() == self) {
                return true;
            }
        }
        return false;
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

                try {
                    queued.task();
                } catch (const std::exception& error) {
                    TE_CHECK(false, "Task in batch {0} threw on worker {1}: {2}", queued.batchId, workerIndex, error.what());
                } catch (...) {
                    TE_CHECK(false, "Task in batch {0} threw a non-std exception on worker {1}", queued.batchId, workerIndex);
                }
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
