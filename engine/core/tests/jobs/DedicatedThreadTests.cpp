#include <TechEngine/core/jobs/DedicatedThread.hpp>
#include <TechEngine/core/jobs/DedicatedThreadContext.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/jobs/ThreadRegistration.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <condition_variable>
#include <exception>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

class DedicatedThreadTestGate {
public:
    bool wait(const std::stop_token stopToken) {
        std::unique_lock lock{m_mutex};
        return m_changed.wait(lock, stopToken, [this] {
            return m_open;
        });
    }

    void open() {
        {
            std::lock_guard const lock{m_mutex};
            m_open = true;
        }
        m_changed.notify_all();
    }

private:
    std::mutex m_mutex;
    std::condition_variable_any m_changed;
    bool m_open = false;
};

static_assert(!std::is_copy_constructible_v<TechEngine::DedicatedThread>);
static_assert(!std::is_copy_assignable_v<TechEngine::DedicatedThread>);
static_assert(std::is_nothrow_move_constructible_v<TechEngine::DedicatedThread>);
static_assert(std::is_nothrow_move_assignable_v<TechEngine::DedicatedThread>);
static_assert(!std::is_copy_constructible_v<TechEngine::DedicatedThreadContext>);
static_assert(!std::is_move_constructible_v<TechEngine::ThreadRegistration>);

TEST_CASE("dedicated threads publish readiness and successful completion", "[core][jobs][dedicated]") {
    TechEngine::JobSystem jobs{1};
    DedicatedThreadTestGate gate;
    std::thread::id executingThread;
    bool returned = false;
    TechEngine::DedicatedThread thread = jobs.createDedicatedThread("DedicatedTest", TechEngine::ThreadRole::Dedicated, [&](TechEngine::DedicatedThreadContext& context) {
        executingThread = std::this_thread::get_id();
        context.signalReady();
        context.signalReady();
        gate.wait(context.stopToken());
        returned = true;
    });

    const TechEngine::ThreadStartupResult startup = thread.waitUntilReady();
    REQUIRE(startup.status == TechEngine::ThreadStartupStatus::Ready);
    REQUIRE_FALSE(startup.failure);
    CHECK(executingThread != std::this_thread::get_id());
    CHECK(thread.completion().status == TechEngine::ThreadCompletionStatus::Running);
    CHECK(thread.joinable());

    gate.open();
    const TechEngine::ThreadCompletionResult completion = thread.waitUntilComplete();
    CHECK(completion.status == TechEngine::ThreadCompletionStatus::Completed);
    CHECK_FALSE(completion.failure);
    CHECK(returned);
    CHECK(jobs.registeredThreads().empty());
    thread.join();
    CHECK_FALSE(thread.joinable());
    CHECK(thread.waitUntilReady().status == TechEngine::ThreadStartupStatus::Ready);
    CHECK(thread.waitUntilComplete().status == TechEngine::ThreadCompletionStatus::Completed);
}

TEST_CASE("dedicated thread failure before readiness wakes the startup waiter", "[core][jobs][dedicated]") {
    TechEngine::JobSystem jobs{1};
    bool throwBeforeReady = false;
    SECTION("exception before readiness") {
        throwBeforeReady = true;
    }
    SECTION("normal return without readiness") {
    }

    TechEngine::DedicatedThread thread = jobs.createDedicatedThread("StartupFailure", TechEngine::ThreadRole::Dedicated, [throwBeforeReady](TechEngine::DedicatedThreadContext&) {
        if (throwBeforeReady) {
            throw std::runtime_error{"startup failed"};
        }
    });

    const TechEngine::ThreadStartupResult startup = thread.waitUntilReady();
    CHECK(startup.status == TechEngine::ThreadStartupStatus::Failed);
    const TechEngine::ThreadCompletionResult completion = thread.waitUntilComplete();
    CHECK(completion.status == TechEngine::ThreadCompletionStatus::Failed);
    if (throwBeforeReady) {
        REQUIRE(startup.failure);
        REQUIRE(completion.failure);
        CHECK_THROWS_WITH(std::rethrow_exception(startup.failure), "startup failed");
        CHECK_THROWS_WITH(std::rethrow_exception(completion.failure), "startup failed");
    } else {
        CHECK_FALSE(startup.failure);
        CHECK_FALSE(completion.failure);
    }
    CHECK(jobs.registeredThreads().empty());
    thread.join();
}

TEST_CASE("dedicated thread failure after readiness remains observable", "[core][jobs][dedicated]") {
    TechEngine::JobSystem jobs{1};
    DedicatedThreadTestGate gate;
    bool throwStandardException = true;
    SECTION("standard exception") {
    }
    SECTION("non-standard exception") {
        throwStandardException = false;
    }

    TechEngine::DedicatedThread thread = jobs.createDedicatedThread("LaterFailure", TechEngine::ThreadRole::Dedicated, [&](TechEngine::DedicatedThreadContext& context) {
        context.signalReady();
        gate.wait(context.stopToken());
        if (throwStandardException) {
            throw std::runtime_error{"work failed"};
        }
        throw 42;
    });

    REQUIRE(thread.waitUntilReady().status == TechEngine::ThreadStartupStatus::Ready);
    CHECK(thread.completion().status == TechEngine::ThreadCompletionStatus::Running);
    gate.open();
    const TechEngine::ThreadCompletionResult completion = thread.waitUntilComplete();
    REQUIRE(completion.status == TechEngine::ThreadCompletionStatus::Failed);
    REQUIRE(completion.failure);
    if (throwStandardException) {
        CHECK_THROWS_WITH(std::rethrow_exception(completion.failure), "work failed");
    } else {
        CHECK_THROWS_AS(std::rethrow_exception(completion.failure), int);
    }
    CHECK(thread.waitUntilReady().status == TechEngine::ThreadStartupStatus::Ready);
    CHECK_FALSE(thread.waitUntilReady().failure);
    CHECK(jobs.registeredThreads().empty());
    thread.join();
}

TEST_CASE("empty dedicated callbacks report startup failure", "[core][jobs][dedicated]") {
    TechEngine::JobSystem jobs{1};
    TechEngine::DedicatedThread thread = jobs.createDedicatedThread("EmptyCallback", TechEngine::ThreadRole::Dedicated, {});
    const TechEngine::ThreadStartupResult startup = thread.waitUntilReady();
    REQUIRE(startup.status == TechEngine::ThreadStartupStatus::Failed);
    REQUIRE(startup.failure);
    CHECK_THROWS_AS(std::rethrow_exception(startup.failure), std::bad_function_call);
    CHECK(thread.waitUntilComplete().status == TechEngine::ThreadCompletionStatus::Failed);
    thread.join();
    CHECK(jobs.registeredThreads().empty());
}

TEST_CASE("dedicated thread stop and join are repeatable", "[core][jobs][dedicated]") {
    TechEngine::JobSystem jobs{1};
    DedicatedThreadTestGate gate;
    bool stopped = false;
    TechEngine::DedicatedThread thread = jobs.createDedicatedThread("StopTest", TechEngine::ThreadRole::Dedicated, [&](TechEngine::DedicatedThreadContext& context) {
        context.signalReady();
        gate.wait(context.stopToken());
        stopped = context.stopToken().stop_requested();
    });

    REQUIRE(thread.waitUntilReady().status == TechEngine::ThreadStartupStatus::Ready);
    thread.requestStop();
    thread.requestStop();
    thread.join();
    thread.join();
    thread.requestStop();
    CHECK(stopped);
    CHECK_FALSE(thread.joinable());
    CHECK(thread.completion().status == TechEngine::ThreadCompletionStatus::Completed);
    CHECK(jobs.registeredThreads().empty());
}

TEST_CASE("empty dedicated handles have no pending work", "[core][jobs][dedicated]") {
    TechEngine::DedicatedThread thread;
    thread.requestStop();
    thread.join();
    thread.join();
    CHECK_FALSE(thread.joinable());
    CHECK(thread.waitUntilReady().status == TechEngine::ThreadStartupStatus::Failed);
    CHECK_FALSE(thread.waitUntilReady().failure);
    CHECK(thread.completion().status == TechEngine::ThreadCompletionStatus::Completed);
    CHECK(thread.waitUntilComplete().status == TechEngine::ThreadCompletionStatus::Completed);
}

TEST_CASE("stopping before readiness still resolves startup and cleans up registration", "[core][jobs][dedicated]") {
    TechEngine::JobSystem jobs{1};
    DedicatedThreadTestGate gate;
    bool stopped = false;
    TechEngine::DedicatedThread thread = jobs.createDedicatedThread("CancelledStartup", TechEngine::ThreadRole::Dedicated, [&](TechEngine::DedicatedThreadContext& context) {
        gate.wait(context.stopToken());
        stopped = context.stopToken().stop_requested();
    });

    thread.requestStop();
    CHECK(thread.waitUntilReady().status == TechEngine::ThreadStartupStatus::Failed);
    CHECK(thread.waitUntilComplete().status == TechEngine::ThreadCompletionStatus::Failed);
    thread.join();
    CHECK(stopped);
    CHECK(jobs.registeredThreads().empty());
}

TEST_CASE("moving dedicated handles preserves active work and results", "[core][jobs][dedicated]") {
    TechEngine::JobSystem jobs{1};
    DedicatedThreadTestGate gate;
    bool stopped = false;
    TechEngine::DedicatedThread original = jobs.createDedicatedThread("MoveTest", TechEngine::ThreadRole::Dedicated, [&](TechEngine::DedicatedThreadContext& context) {
        context.signalReady();
        gate.wait(context.stopToken());
        stopped = context.stopToken().stop_requested();
    });
    REQUIRE(original.waitUntilReady().status == TechEngine::ThreadStartupStatus::Ready);

    TechEngine::DedicatedThread moved{std::move(original)};
    CHECK_FALSE(original.joinable());
    original.requestStop();
    original.join();
    CHECK(original.waitUntilReady().status == TechEngine::ThreadStartupStatus::Failed);
    CHECK(moved.waitUntilReady().status == TechEngine::ThreadStartupStatus::Ready);
    CHECK(moved.completion().status == TechEngine::ThreadCompletionStatus::Running);
    moved.requestStop();
    moved.join();
    CHECK(stopped);

    TechEngine::DedicatedThread completed{std::move(moved)};
    CHECK_FALSE(completed.joinable());
    CHECK(completed.completion().status == TechEngine::ThreadCompletionStatus::Completed);
    CHECK(completed.waitUntilReady().status == TechEngine::ThreadStartupStatus::Ready);
}

TEST_CASE("move assignment stops the previous thread before replacing its state", "[core][jobs][dedicated]") {
    TechEngine::JobSystem jobs{1};
    DedicatedThreadTestGate oldGate;
    DedicatedThreadTestGate newGate;
    bool oldStopped = false;
    bool newStopped = false;
    TechEngine::DedicatedThread target = jobs.createDedicatedThread("OldThread", TechEngine::ThreadRole::Dedicated, [&](TechEngine::DedicatedThreadContext& context) {
        context.signalReady();
        oldGate.wait(context.stopToken());
        oldStopped = context.stopToken().stop_requested();
    });
    TechEngine::DedicatedThread source = jobs.createDedicatedThread("NewThread", TechEngine::ThreadRole::Dedicated, [&](TechEngine::DedicatedThreadContext& context) {
        context.signalReady();
        newGate.wait(context.stopToken());
        newStopped = context.stopToken().stop_requested();
    });
    REQUIRE(target.waitUntilReady().status == TechEngine::ThreadStartupStatus::Ready);
    REQUIRE(source.waitUntilReady().status == TechEngine::ThreadStartupStatus::Ready);

    target = std::move(source);
    CHECK(oldStopped);
    CHECK_FALSE(source.joinable());
    CHECK(target.completion().status == TechEngine::ThreadCompletionStatus::Running);
    const std::vector<TechEngine::ThreadInfo> registered = jobs.registeredThreads();
    REQUIRE(registered.size() == 1);
    CHECK(registered.front().name == "NewThread");

    target.requestStop();
    target.join();
    CHECK(newStopped);
    CHECK(jobs.registeredThreads().empty());
}

TEST_CASE("dedicated handle destruction requests stop and joins", "[core][jobs][dedicated]") {
    TechEngine::JobSystem jobs{1};
    DedicatedThreadTestGate gate;
    bool stopped = false;
    {
        TechEngine::DedicatedThread thread = jobs.createDedicatedThread("DestructorTest", TechEngine::ThreadRole::Dedicated, [&](TechEngine::DedicatedThreadContext& context) {
            context.signalReady();
            gate.wait(context.stopToken());
            stopped = context.stopToken().stop_requested();
        });
        REQUIRE(thread.waitUntilReady().status == TechEngine::ThreadStartupStatus::Ready);
    }
    CHECK(stopped);
    CHECK(jobs.registeredThreads().empty());
}

TEST_CASE("thread registrations copy metadata and clean up after completion", "[core][jobs][dedicated]") {
    TechEngine::JobSystem jobs{1};
    DedicatedThreadTestGate gate;
    std::string name = "NamedThread";
    TechEngine::ThreadRole role = TechEngine::ThreadRole::Dedicated;
    TechEngine::DedicatedThread thread = jobs.createDedicatedThread(name, role, [&](TechEngine::DedicatedThreadContext& context) {
        context.signalReady();
        gate.wait(context.stopToken());
    });
    name = "ChangedName";
    role = TechEngine::ThreadRole::Host;
    REQUIRE(thread.waitUntilReady().status == TechEngine::ThreadStartupStatus::Ready);

    std::vector<TechEngine::ThreadInfo> snapshot = jobs.registeredThreads();
    REQUIRE(snapshot.size() == 1);
    CHECK(snapshot.front().name == "NamedThread");
    CHECK(snapshot.front().role != role);
    CHECK(snapshot.front().id != std::this_thread::get_id());
    snapshot.front().name = "ChangedSnapshot";
    CHECK(jobs.registeredThreads().front().name == "NamedThread");

    gate.open();
    thread.join();
    CHECK(jobs.registeredThreads().empty());
    CHECK(snapshot.front().name == "ChangedSnapshot");
}

TEST_CASE("host registration is scoped to the calling thread", "[core][jobs][dedicated]") {
    TechEngine::JobSystem jobs{1};
    {
        const TechEngine::ThreadRegistration registration = jobs.registerCurrentThread("TestHost", TechEngine::ThreadRole::Host);
        const std::vector<TechEngine::ThreadInfo> snapshot = jobs.registeredThreads();
        REQUIRE(snapshot.size() == 1);
        CHECK(snapshot.front().id == std::this_thread::get_id());
        CHECK(snapshot.front().name == "TestHost");
        CHECK(snapshot.front().role == TechEngine::ThreadRole::Host);
    }
    CHECK(jobs.registeredThreads().empty());

    try {
        const TechEngine::ThreadRegistration registration = jobs.registerCurrentThread("UnwindingHost", TechEngine::ThreadRole::Host);
        throw std::runtime_error{"unwind"};
    } catch (const std::runtime_error&) {
    }
    CHECK(jobs.registeredThreads().empty());
}

TEST_CASE("duplicate registration is rejected without removing the existing scope", "[core][jobs][dedicated]") {
    TechEngine::JobSystem jobs{1};
    const TechEngineTests::FatalAssertGuard guard;
    const TechEngine::ThreadRegistration registration = jobs.registerCurrentThread("OriginalHost", TechEngine::ThreadRole::Host);
    REQUIRE_THROWS_AS(jobs.registerCurrentThread("DuplicateHost", TechEngine::ThreadRole::Host), TechEngineTests::AssertFired);
    const std::vector<TechEngine::ThreadInfo> snapshot = jobs.registeredThreads();
    REQUIRE(snapshot.size() == 1);
    CHECK(snapshot.front().name == "OriginalHost");
}

TEST_CASE("dedicated threads reject joining themselves", "[core][jobs][dedicated]") {
    TechEngine::JobSystem jobs{1};
    const TechEngineTests::AssertHandlerGuard guard;
    DedicatedThreadTestGate gate;
    bool remainedJoinable = false;
    TechEngine::DedicatedThread thread;
    thread = jobs.createDedicatedThread("SelfJoin", TechEngine::ThreadRole::Dedicated, [&](TechEngine::DedicatedThreadContext& context) {
        context.signalReady();
        if (gate.wait(context.stopToken())) {
            thread.join();
            remainedJoinable = thread.joinable();
        }
    });

    REQUIRE(thread.waitUntilReady().status == TechEngine::ThreadStartupStatus::Ready);
    gate.open();
    thread.waitUntilComplete();
    thread.join();
    REQUIRE(TechEngineTests::g_fired.size() == 1);
    CHECK(TechEngineTests::g_fired.front() == TechEngine::AssertKind::Ensure);
    CHECK(remainedJoinable);
}

TEST_CASE("pool shutdown leaves dedicated thread ownership with its caller", "[core][jobs][dedicated]") {
    TechEngine::JobSystem jobs{1};
    DedicatedThreadTestGate gate;
    TechEngine::DedicatedThread thread = jobs.createDedicatedThread("SurvivingThread", TechEngine::ThreadRole::Dedicated, [&](TechEngine::DedicatedThreadContext& context) {
        context.signalReady();
        gate.wait(context.stopToken());
    });

    REQUIRE(thread.waitUntilReady().status == TechEngine::ThreadStartupStatus::Ready);
    jobs.shutdown();
    CHECK(thread.joinable());
    CHECK(thread.completion().status == TechEngine::ThreadCompletionStatus::Running);
    CHECK(jobs.registeredThreads().size() == 1);

    thread.requestStop();
    thread.join();
    CHECK(jobs.registeredThreads().empty());
}
