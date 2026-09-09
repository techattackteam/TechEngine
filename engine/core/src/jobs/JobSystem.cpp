#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>

#include <jobs/DedicatedThreadState.hpp>
#include <jobs/RegisteredThread.hpp>

#include <exception>
#include <format>
#include <memory>
#include <stop_token>
#include <string>
#include <utility>

namespace TechEngine {
    JobSystem::JobSystem(const std::size_t workerCount) {
        std::size_t count = workerCount;
        if (!TE_ENSURE(count > 0, "A job system needs at least one worker; starting one")) {
            count = 1;
        }

        m_running = true;
        m_workers.reserve(count);

        try {
            for (std::size_t i = 0; i < count; i++) {
                m_workers.push_back(createRegisteredThread<std::thread>(
                    [this, name = std::format("TEWorker{0}", i)] {
                        return registerCurrentThread(name, ThreadRole::PoolWorker);
                    },
                    [this, i](const std::stop_token stopToken) {
                        workerMain(i);
                    },
                    [](const std::exception_ptr failure) {
                        if (failure) {
                            std::terminate();
                        }
                    }));
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

        if (!TE_ENSURE(id != 0, "Work submitted after the job system shut down")) {
            return {};
        }

        m_workAvailable.notify_all();
        return BatchId{id};
    }

    void JobSystem::wait(const BatchId batch) {
        if (!batch.valid()) {
            return;
        }

        if (!TE_ENSURE(!isWorkerThread(), "wait() called from a pool worker; barriers never run on workers")) {
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

    DedicatedThread JobSystem::createDedicatedThread(std::string name, const ThreadRole role, std::function<void(DedicatedThreadContext&)> entry) {
        DedicatedThread thread;
        thread.m_state = std::make_unique<DedicatedThreadContext::State>();
        DedicatedThreadContext::State* const state = thread.m_state.get();

        try {
            thread.m_thread = createRegisteredThread<std::jthread>(
                [this, name = std::move(name), role] {
                    return registerCurrentThread(name, role);
                },
                [state, entry = std::move(entry)](const std::stop_token stopToken) {
                    DedicatedThreadContext context{*state, stopToken};
                    entry(context);
                },
                [state](const std::exception_ptr failure) {
                    state->finish(failure);
                });
        } catch (...) {
            state->finish(std::current_exception());
        }

        return thread;
    }

    ThreadRegistration JobSystem::registerCurrentThread(std::string name, const ThreadRole role) {
        return ThreadRegistration{*this, std::move(name), role};
    }

    std::vector<ThreadInfo> JobSystem::registeredThreads() const {
        std::lock_guard const lock{m_registryMutex};
        std::vector<ThreadInfo> threads;
        threads.reserve(m_registeredThreads.size());
        for (const auto& entry: m_registeredThreads) {
            threads.push_back(entry.second);
        }
        return threads;
    }

    bool JobSystem::isWorkerThread() const {
        std::lock_guard const lock{m_registryMutex};
        const auto it = m_registeredThreads.find(std::this_thread::get_id());
        return it != m_registeredThreads.end() && it->second.role == ThreadRole::PoolWorker;
    }

    void JobSystem::workerMain(const std::size_t workerIndex) {
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
