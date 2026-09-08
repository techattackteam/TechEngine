#pragma once

#include <TechEngine/core/jobs/DedicatedThread.hpp>
#include <TechEngine/core/jobs/ThreadRegistration.hpp>

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <span>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace TechEngine {
    using Task = std::function<void()>;

    struct BatchId {
        constexpr BatchId() = default;

        constexpr explicit BatchId(std::uint64_t value) : m_value{value} {
        }

        constexpr std::uint64_t value() const {
            return m_value;
        }

        constexpr bool valid() const {
            return m_value != 0;
        }

        bool operator==(const BatchId&) const = default;

    private:
        std::uint64_t m_value = 0;
    };

    class JobSystem {
    public:
        static constexpr std::size_t DEFAULT_WORKER_COUNT = 4;

    private:
        friend class ThreadRegistration;

        struct QueuedTask {
            Task task;
            std::uint64_t batchId = 0;
        };

        std::vector<std::thread> m_workers;
        std::mutex m_mutex;
        std::mutex m_shutdownMutex;
        std::condition_variable m_workAvailable;
        std::condition_variable m_batchComplete;
        std::deque<QueuedTask> m_queue;
        std::unordered_map<std::uint64_t, std::size_t> m_pendingBatches;
        std::uint64_t m_nextBatchId = 1;
        bool m_running = false;

        mutable std::mutex m_registryMutex;
        std::unordered_map<std::thread::id, ThreadInfo> m_registeredThreads;

    public:
        explicit JobSystem(std::size_t workerCount = DEFAULT_WORKER_COUNT);

        ~JobSystem();

        JobSystem(const JobSystem&) = delete;

        JobSystem& operator=(const JobSystem&) = delete;

        JobSystem(JobSystem&&) = delete;

        JobSystem& operator=(JobSystem&&) = delete;

        // Moves the callables out of `tasks`; the caller's span holds empty Tasks afterwards.
        BatchId submit(std::span<Task> tasks);

        void wait(BatchId batch);

        void shutdown();

        std::size_t workerCount() const;

        // This JobSystem must outlive the returned handles and registrations.
        DedicatedThread createDedicatedThread(std::string name, ThreadRole role, std::function<void(DedicatedThreadContext&)> entry);

        ThreadRegistration registerCurrentThread(std::string name, ThreadRole role);

        std::vector<ThreadInfo> registeredThreads() const;

    private:
        void workerMain(std::size_t workerIndex);

        bool isWorkerThread() const;
    };
}
