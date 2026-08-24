#pragma once

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <span>
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
        struct QueuedTask {
            Task task;
            std::uint64_t batchId = 0;
        };

        std::vector<std::thread> m_workers;
        std::mutex m_mutex;
        std::condition_variable m_workAvailable;
        std::condition_variable m_batchComplete;
        std::deque<QueuedTask> m_queue;
        std::unordered_map<std::uint64_t, std::size_t> m_pendingBatches;
        std::uint64_t m_nextBatchId = 1;
        bool m_running = false;

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

    private:
        void workerMain(std::size_t workerIndex);
    };
}
