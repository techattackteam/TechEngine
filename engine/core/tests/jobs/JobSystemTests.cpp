#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>

#include "../events/AssertCapture.hpp"
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>

TEST_CASE("the pool reports its worker count", "[core][jobs]") {
    const TechEngine::JobSystem jobs{1};

    REQUIRE(jobs.workerCount() == 1);
}

TEST_CASE("a default pool starts the decided number of workers", "[core][jobs]") {
    const TechEngine::JobSystem jobs;

    REQUIRE(jobs.workerCount() == TechEngine::JobSystem::DEFAULT_WORKER_COUNT);
    REQUIRE(TechEngine::JobSystem::DEFAULT_WORKER_COUNT == 4);
}

TEST_CASE("every task in a batch runs exactly once", "[core][jobs]") {
    TechEngine::JobSystem jobs{1};

    constexpr std::size_t TASK_COUNT = 8;
    std::array<std::atomic<int>, TASK_COUNT> counters{};

    std::vector<std::function<void()>> tasks;
    tasks.reserve(TASK_COUNT);
    for (std::size_t i = 0; i < TASK_COUNT; i++) {
        tasks.emplace_back([&counters, i] {
            counters[i].fetch_add(1);
        });
    }

    const TechEngine::BatchId batch = jobs.submit(tasks);
    jobs.wait(batch);

    for (const std::atomic<int>& counter: counters) {
        REQUIRE(counter.load() == 1);
    }
}

TEST_CASE("wait returns only after the batch completes", "[core][jobs]") {
    TechEngine::JobSystem jobs{1};

    std::atomic<bool> finished = false;

    // The sleep is the test: a wait that returns early still passes without it.
    std::array<std::function<void()>, 1> tasks{[&finished] {
        std::this_thread::sleep_for(std::chrono::milliseconds{20});
        finished.store(true);
    }};

    const TechEngine::BatchId batch = jobs.submit(tasks);
    jobs.wait(batch);

    REQUIRE(finished.load());
}

TEST_CASE("tasks execute on a worker thread, not on the caller", "[core][jobs]") {
    TechEngine::JobSystem jobs{1};

    std::atomic<std::thread::id> ranOn{};
    std::array<std::function<void()>, 1> tasks{[&ranOn] {
        ranOn.store(std::this_thread::get_id());
    }};

    const TechEngine::BatchId batch = jobs.submit(tasks);
    jobs.wait(batch);

    REQUIRE(ranOn.load() != std::thread::id{});
    REQUIRE(ranOn.load() != std::this_thread::get_id());
}

TEST_CASE("submitting after shutdown is checked and runs nothing", "[core][jobs]") {
    TechEngine::JobSystem jobs{1};
    jobs.shutdown();

    std::atomic<int> ran = 0;
    std::array<std::function<void()>, 1> tasks{[&ran] {
        ran.fetch_add(1);
    }};

    const TechEngineTests::AssertHandlerGuard guard;
    const TechEngine::BatchId batch = jobs.submit(tasks);

    REQUIRE(TechEngineTests::g_fired.size() == 1);
    REQUIRE(TechEngineTests::g_fired.front() == TechEngine::AssertKind::Check);
    REQUIRE_FALSE(batch.valid());
    REQUIRE(ran.load() == 0);
}

TEST_CASE("a partitioned sum across four workers reaches the exact total", "[core][jobs]") {
    constexpr std::size_t WORKER_COUNT = 4;
    constexpr std::uint64_t SUM_LIMIT = 100000;
    constexpr std::uint64_t SUM_EXPECTED = SUM_LIMIT * (SUM_LIMIT + 1) / 2;
    constexpr std::uint64_t CHUNK = SUM_LIMIT / WORKER_COUNT;

    TechEngine::JobSystem jobs{WORKER_COUNT};
    REQUIRE(jobs.workerCount() == WORKER_COUNT);

    std::array<std::uint64_t, WORKER_COUNT> partials{};

    std::vector<TechEngine::Task> tasks;
    tasks.reserve(WORKER_COUNT);
    for (std::size_t j = 0; j < WORKER_COUNT; j++) {
        const std::uint64_t begin = j * CHUNK + 1;
        const std::uint64_t end = j + 1 == WORKER_COUNT ? SUM_LIMIT : (j + 1) * CHUNK;
        tasks.emplace_back([&partials, j, begin, end] {
            std::uint64_t sum = 0;
            for (std::uint64_t k = begin; k <= end; k++) {
                sum += k;
            }
            partials[j] = sum;
        });
    }

    const TechEngine::BatchId batch = jobs.submit(tasks);
    jobs.wait(batch);

    std::uint64_t total = 0;
    for (const std::uint64_t partial: partials) {
        total += partial;
    }

    REQUIRE(total == SUM_EXPECTED);
}

TEST_CASE("a multi-worker pool runs many batches back to back", "[core][jobs]") {
    constexpr std::size_t WORKER_COUNT = 4;
    constexpr std::size_t BATCH_COUNT = 100;
    constexpr std::size_t TASKS_PER_BATCH = 8;

    TechEngine::JobSystem jobs{WORKER_COUNT};

    std::atomic<int> ran = 0;

    for (std::size_t batchIndex = 0; batchIndex < BATCH_COUNT; batchIndex++) {
        std::vector<TechEngine::Task> tasks;
        tasks.reserve(TASKS_PER_BATCH);
        for (std::size_t j = 0; j < TASKS_PER_BATCH; j++) {
            tasks.emplace_back([&ran] {
                ran.fetch_add(1);
            });
        }

        const TechEngine::BatchId batch = jobs.submit(tasks);
        jobs.wait(batch);
    }

    REQUIRE(ran.load() == static_cast<int>(BATCH_COUNT * TASKS_PER_BATCH));
}
