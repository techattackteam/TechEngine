#include <TechEngine/app/Command.hpp>
#include <TechEngine/app/CommandQueue.hpp>
#include <TechEngine/app/SimulationError.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <memory>
#include <system_error>
#include <thread>
#include <vector>

using TechEngine::CommandQueue;
using TechEngine::SimulationError;

class QueueProbeCommand final : public TechEngine::Command {
private:
    int m_id;
    std::atomic<int>& m_destroyed;

public:
    QueueProbeCommand(const int id, std::atomic<int>& destroyed) : m_id(id), m_destroyed(destroyed) {
    }

    ~QueueProbeCommand() override {
        m_destroyed++;
    }

    void apply(TechEngine::Scene&) override {
    }

    int id() const {
        return m_id;
    }
};

static std::error_code submitProbe(CommandQueue& queue, const int id, std::atomic<int>& destroyed) {
    return queue.submit(std::make_unique<QueueProbeCommand>(id, destroyed));
}

static std::vector<int> consumeIds(CommandQueue& queue) {
    std::vector<std::unique_ptr<TechEngine::Command>> batch;
    queue.consume(batch);
    std::vector<int> ids;
    for (const std::unique_ptr<TechEngine::Command>& command: batch) {
        ids.push_back(static_cast<const QueueProbeCommand&>(*command).id());
    }
    return ids;
}

TEST_CASE("a command queue refuses a zero capacity and a null command", "[app][commands]") {
    const TechEngineTests::FatalAssertGuard guard;
    REQUIRE_THROWS_AS(CommandQueue{0}, TechEngineTests::AssertFired);

    CommandQueue queue{1};
    queue.open();
    REQUIRE_THROWS_AS(queue.submit(nullptr), TechEngineTests::AssertFired);
    CHECK(consumeIds(queue).empty());
}

TEST_CASE("a new command queue rejects every command until it is opened", "[app][commands]") {
    std::atomic<int> destroyed = 0;
    CommandQueue queue{4};

    const std::error_code beforeOpenError = submitProbe(queue, 1, destroyed);

    CHECK(beforeOpenError == SimulationError::CommandQueueClosed);
    CHECK(destroyed.load() == 1);
    CHECK(consumeIds(queue).empty());

    queue.open();
    const std::error_code openedError = submitProbe(queue, 2, destroyed);
    CHECK_FALSE(openedError);
    CHECK(consumeIds(queue) == std::vector<int>{2});
}

TEST_CASE("a command queue hands its commands back in submission order", "[app][commands]") {
    std::atomic<int> destroyed = 0;
    CommandQueue queue{4};
    queue.open();

    for (int id = 1; id <= 4; id++) {
        const std::error_code submitError = submitProbe(queue, id, destroyed);
        REQUIRE_FALSE(submitError);
    }

    CHECK(consumeIds(queue) == std::vector<int>{1, 2, 3, 4});
    CHECK(consumeIds(queue).empty());
    CHECK(destroyed.load() == 4);
}

TEST_CASE("a full command queue rejects the new command and keeps the one it accepted", "[app][commands]") {
    std::atomic<int> destroyed = 0;
    CommandQueue queue{1};
    queue.open();
    REQUIRE(queue.capacity() == 1);

    const std::error_code firstError = submitProbe(queue, 1, destroyed);
    const std::error_code secondError = submitProbe(queue, 2, destroyed);

    REQUIRE_FALSE(firstError);
    CHECK(secondError == SimulationError::CommandQueueFull);
    CHECK(destroyed.load() == 1);
    CHECK(consumeIds(queue) == std::vector<int>{1});

    const std::error_code afterConsumeError = submitProbe(queue, 3, destroyed);
    CHECK_FALSE(afterConsumeError);
    CHECK(consumeIds(queue) == std::vector<int>{3});
}

TEST_CASE("a closed command queue rejects new commands but keeps what it accepted until it opens again", "[app][commands]") {
    std::atomic<int> destroyed = 0;
    CommandQueue queue{4};
    queue.open();
    const std::error_code acceptedError = submitProbe(queue, 1, destroyed);
    REQUIRE_FALSE(acceptedError);

    queue.close();
    queue.close();
    const std::error_code closedError = submitProbe(queue, 2, destroyed);

    CHECK(closedError == SimulationError::CommandQueueClosed);
    CHECK(destroyed.load() == 1);
    CHECK(consumeIds(queue) == std::vector<int>{1});

    queue.open();
    const std::error_code reopenedError = submitProbe(queue, 3, destroyed);
    CHECK_FALSE(reopenedError);
    CHECK(consumeIds(queue) == std::vector<int>{3});
}

TEST_CASE("a command queue keeps each producer's order while a consumer drains it", "[app][commands]") {
    constexpr int PRODUCER_COUNT = 2;
    constexpr int COMMANDS_PER_PRODUCER = 2000;
    constexpr int PRODUCER_STRIDE = 100000;
    std::atomic<int> destroyed = 0;
    CommandQueue queue{8};
    queue.open();
    std::atomic<int> rejectedOtherThanFull = 0;

    std::vector<std::jthread> producers;
    for (int producer = 0; producer < PRODUCER_COUNT; producer++) {
        producers.emplace_back([&, producer] {
            for (int i = 0; i < COMMANDS_PER_PRODUCER; i++) {
                const int id = producer * PRODUCER_STRIDE + i;
                while (true) {
                    const std::error_code submitError = submitProbe(queue, id, destroyed);
                    if (!submitError) {
                        break;
                    }
                    if (submitError != SimulationError::CommandQueueFull) {
                        rejectedOtherThanFull++;
                        return;
                    }
                    std::this_thread::yield();
                }
            }
        });
    }

    std::vector<int> received;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (received.size() < static_cast<std::size_t>(PRODUCER_COUNT * COMMANDS_PER_PRODUCER) && std::chrono::steady_clock::now() < deadline) {
        const std::vector<int> batch = consumeIds(queue);
        received.insert(received.end(), batch.begin(), batch.end());
        std::this_thread::yield();
    }
    producers.clear();

    CHECK(rejectedOtherThanFull.load() == 0);
    REQUIRE(received.size() == static_cast<std::size_t>(PRODUCER_COUNT * COMMANDS_PER_PRODUCER));
    std::vector<int> next(PRODUCER_COUNT, 0);
    for (const int id: received) {
        const auto producer = static_cast<std::size_t>(id / PRODUCER_STRIDE);
        REQUIRE(producer < next.size());
        CHECK(id % PRODUCER_STRIDE == next[producer]);
        next[producer]++;
    }
}
