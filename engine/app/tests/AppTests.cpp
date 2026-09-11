#include <TechEngine/app/App.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/platform/files/FileResult.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <vector>

namespace {
    class ProbeApp : public TechEngine::App {
    public:
        explicit ProbeApp(TechEngine::Role role) : App(role) {
        }

        void init() override {
            registeredDuringInit = mainRegistered();
            if (failInit) {
                throw std::runtime_error{"init failed"};
            }
        }

        void shutdown() override {
            registeredDuringShutdown = mainRegistered();
        }

        bool shouldClose() const override {
            return true;
        }

        TechEngine::Role loopRole() const {
            return m_simulationThread.simulationContext().role;
        }

        TechEngine::MountTable& table() {
            return m_mounts;
        }

        const TechEngine::EngineContext& context() const {
            return m_engine;
        }

        bool mainRegistered() const {
            for (const TechEngine::ThreadInfo& thread: m_jobs.registeredThreads()) {
                if (thread.id == std::this_thread::get_id() && thread.role == TechEngine::ThreadRole::Main && thread.name == "TEMain") {
                    return true;
                }
            }
            return false;
        }

        bool failInit = false;
        bool registeredDuringInit = false;
        bool registeredDuringShutdown = false;
    };
}

TEST_CASE("App scopes main registration across initialization and shutdown", "[app]") {
    ProbeApp app{TechEngine::Role::DedicatedServer};
    CHECK_FALSE(app.mainRegistered());
    REQUIRE(app.run() == 0);
    CHECK(app.registeredDuringInit);
    CHECK(app.registeredDuringShutdown);
    CHECK_FALSE(app.mainRegistered());
    REQUIRE(app.run() == 0);
    CHECK_FALSE(app.mainRegistered());
}

TEST_CASE("App removes main registration when initialization throws", "[app]") {
    ProbeApp app{TechEngine::Role::DedicatedServer};
    app.failInit = true;
    CHECK(app.run() != 0);
    CHECK(app.registeredDuringInit);
    CHECK_FALSE(app.mainRegistered());
}

TEST_CASE("the subclass's role reaches the loop App owns", "[app]") {
    const ProbeApp app{TechEngine::Role::DedicatedServer};

    REQUIRE(app.loopRole() == TechEngine::Role::DedicatedServer);
}

TEST_CASE("a mount added after construction is visible through App's context", "[app]") {
    ProbeApp app{TechEngine::Role::Client};

    app.table().mount("scratch", std::filesystem::temp_directory_path());

    std::filesystem::path resolved;
    const TechEngine::FileResult result = app.context().files.resolve("scratch://no-such-file", resolved);

    // NotFound means the context saw the mount and the file was absent, which is the point.
    // NoMount would mean EngineContext had snapshotted the table instead of referencing it,
    // and init() could then never mount anything the loop reads.
    REQUIRE(result == TechEngine::FileResult::NotFound);
}

using namespace std::chrono_literals;

class AppLifecycleSignal {
public:
    void set() {
        {
            std::lock_guard const lock{m_mutex};
            m_set = true;
        }
        m_changed.notify_all();
    }

    bool wait(std::chrono::milliseconds timeout = 5000ms) {
        std::unique_lock lock{m_mutex};
        return m_changed.wait_for(lock, timeout, [this] {
            return m_set;
        });
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_changed;
    bool m_set = false;
};

struct AppLifecycleEvent {
    std::string_view name;
    std::thread::id thread;
    std::uint64_t tick = 0;
};

class AppLifecycleProbe : public TechEngine::App {
public:
    AppLifecycleProbe() : App(TechEngine::Role::DedicatedServer) {
    }

    std::function<void()> onInit;
    std::function<void()> onMainUpdate;
    std::function<void()> onSimulationInit;
    std::function<void(const TechEngine::SimulationContext&)> onFixedUpdate;
    std::function<void(const TechEngine::SimulationContext&)> onPublishSnapshot;
    std::function<void()> onSimulationShutdown;
    std::function<void()> onShutdown;
    AppLifecycleSignal mainEntered;
    AppLifecycleSignal simulationStopped;
    bool dedicatedJoinedBeforeShutdown = false;

    std::vector<AppLifecycleEvent> events() const {
        std::lock_guard const lock{m_eventsMutex};
        return m_events;
    }

    std::vector<TechEngine::ThreadInfo> threads() const {
        return m_jobs.registeredThreads();
    }

    std::uint64_t simulationTick() const {
        return timingMetrics().simulation.tick;
    }

    std::uint64_t diagnosticFrame() const {
        return m_clock.frame();
    }

    TechEngine::ThreadCompletionResult simulationCompletion() const {
        return m_simulationThread.completion();
    }

protected:
    void init() override {
        record("init");
        if (onInit) {
            onInit();
        }
    }

    void mainUpdate() override {
        record("mainUpdate");
        mainEntered.set();
        if (onMainUpdate) {
            onMainUpdate();
        } else {
            App::mainUpdate();
        }
    }

    bool shouldClose() const override {
        record("shouldClose");
        return false;
    }

    void simulationInit() override {
        record("simulationInit");
        if (onSimulationInit) {
            onSimulationInit();
        }
    }

    void fixedUpdate(const TechEngine::SimulationContext& simulation) override {
        record("fixedUpdate", simulation.tick);
        if (onFixedUpdate) {
            onFixedUpdate(simulation);
        }
    }

    void publishSnapshot(const TechEngine::SimulationContext& simulation) override {
        record("publishSnapshot", simulation.tick);
        if (onPublishSnapshot) {
            onPublishSnapshot(simulation);
        }
    }

    void simulationShutdown() override {
        record("simulationShutdown");
        if (onSimulationShutdown) {
            onSimulationShutdown();
        }
        simulationStopped.set();
    }

    void shutdown() override {
        const auto registered = threads();
        dedicatedJoinedBeforeShutdown = std::none_of(registered.begin(), registered.end(), [](const auto& thread) {
            return thread.role == TechEngine::ThreadRole::Dedicated;
        });
        record("shutdown");
        if (onShutdown) {
            onShutdown();
        }
    }

private:
    void record(std::string_view name, std::uint64_t tick = 0) const {
        std::lock_guard const lock{m_eventsMutex};
        m_events.push_back({name, std::this_thread::get_id(), tick});
    }

    mutable std::mutex m_eventsMutex;
    mutable std::vector<AppLifecycleEvent> m_events;
};

static std::size_t eventCount(const AppLifecycleProbe& app, std::string_view name) {
    const auto events = app.events();
    return static_cast<std::size_t>(std::ranges::count_if(events, [name](const auto& event) {
        return event.name == name;
    }));
}

static void checkStopped(const AppLifecycleProbe& app) {
    const auto events = app.events();
    REQUIRE(events.size() >= 2);
    const auto simulationShutdown = std::find_if(events.begin(), events.end(), [](const auto& event) {
        return event.name == "simulationShutdown";
    });
    REQUIRE(simulationShutdown != events.end());
    CHECK(events.back().name == "shutdown");
    CHECK(std::none_of(simulationShutdown + 1, events.end(), [](const auto& event) {
        return event.name == "fixedUpdate" || event.name == "publishSnapshot";
    }));
    CHECK(app.dedicatedJoinedBeforeShutdown);
    const auto threads = app.threads();
    CHECK(std::none_of(threads.begin(), threads.end(), [](const auto& thread) {
        return thread.role != TechEngine::ThreadRole::PoolWorker;
    }));
}

TEST_CASE("App advances simulation while the main thread is stalled", "[app][lifecycle]") {
    AppLifecycleProbe app;
    AppLifecycleSignal releaseMain;
    bool mainReleased = true;
    bool mainEntered = false;
    bool advanced = false;
    std::uint64_t before = 0;
    std::uint64_t after = 0;
    app.onMainUpdate = [&] {
        mainReleased = releaseMain.wait(10000ms) && mainReleased;
    };

    std::jthread controller{[&] {
        mainEntered = app.mainEntered.wait();
        if (mainEntered) {
            before = app.simulationTick();
            const auto deadline = std::chrono::steady_clock::now() + 5s;
            do {
                after = app.simulationTick();
                advanced = after >= before + 3;
                if (!advanced) {
                    std::this_thread::sleep_for(1ms);
                }
            } while (!advanced && std::chrono::steady_clock::now() < deadline);
        }
        app.requestStop();
        releaseMain.set();
    }};

    const int result = app.run();
    controller.join();
    CHECK(result == 0);
    CHECK(mainEntered);
    CHECK(mainReleased);
    CHECK(advanced);
    CHECK(after >= before + 3);
    checkStopped(app);
}

TEST_CASE("App runs hooks on their owning threads and publishes after each tick batch", "[app][lifecycle]") {
    AppLifecycleProbe app;
    const auto mainThread = std::this_thread::get_id();
    app.onPublishSnapshot = [&](const TechEngine::SimulationContext& simulation) {
        if (simulation.tick >= 3) {
            app.requestStop();
        }
    };

    REQUIRE(app.run() == 0);
    const auto events = app.events();
    const auto simulationInit = std::find_if(events.begin(), events.end(), [](const auto& event) {
        return event.name == "simulationInit";
    });
    REQUIRE(simulationInit != events.end());
    const auto simulationThread = simulationInit->thread;
    CHECK(simulationThread != mainThread);
    CHECK(events.front().name == "init");
    CHECK(eventCount(app, "mainUpdate") > 0);
    CHECK(eventCount(app, "shouldClose") > 0);
    CHECK(eventCount(app, "fixedUpdate") >= 3);
    CHECK(eventCount(app, "publishSnapshot") >= 2);

    std::uint64_t latestFixedTick = 0;
    std::uint64_t lastPublishedTick = 0;
    std::size_t publications = 0;
    for (const auto& event: events) {
        if (event.name == "init" || event.name == "mainUpdate" || event.name == "shouldClose" || event.name == "shutdown") {
            CHECK(event.thread == mainThread);
        } else {
            CHECK(event.thread == simulationThread);
        }
        if (event.name == "fixedUpdate") {
            CHECK(publications > 0);
            CHECK(event.tick == latestFixedTick + 1);
            latestFixedTick = event.tick;
        } else if (event.name == "publishSnapshot") {
            CHECK(event.tick == latestFixedTick);
            if (publications > 0) {
                CHECK(event.tick > lastPublishedTick);
            }
            lastPublishedTick = event.tick;
            publications++;
        }
    }
    checkStopped(app);
}

TEST_CASE("App's primary simulation advances the diagnostic counter once per tick", "[app][lifecycle]") {
    AppLifecycleProbe app;
    app.onPublishSnapshot = [&](const TechEngine::SimulationContext& simulation) {
        if (simulation.tick >= 5) {
            app.requestStop();
        }
    };

    REQUIRE(app.run() == 0);
    const TechEngine::TimingMetrics timing = app.timingMetrics();
    CHECK(timing.simulation.tick >= 5);
    CHECK(app.diagnosticFrame() == timing.simulation.tick);
    CHECK(eventCount(app, "fixedUpdate") == timing.simulation.tick);
    CHECK_FALSE(timing.render.has_value());
}

TEST_CASE("App unwinds only successfully initialized stages", "[app][lifecycle]") {
    AppLifecycleProbe app;
    bool mainFails = false;
    bool publicationFails = false;
    SECTION("main initialization fails") {
        mainFails = true;
        app.onInit = [] {
            throw std::runtime_error{"main startup failed"};
        };
    }
    SECTION("simulation initialization fails") {
        app.onSimulationInit = [] {
            throw std::runtime_error{"simulation startup failed"};
        };
    }
    SECTION("the initial publication fails") {
        publicationFails = true;
        app.onPublishSnapshot = [](const TechEngine::SimulationContext&) {
            throw std::runtime_error{"initial publication failed"};
        };
    }

    CHECK(app.run() != 0);
    CHECK(eventCount(app, "init") == 1);
    CHECK(eventCount(app, "simulationInit") == (mainFails ? 0 : 1));
    CHECK(eventCount(app, "publishSnapshot") == (publicationFails ? 1 : 0));
    CHECK(eventCount(app, "fixedUpdate") == 0);
    CHECK(eventCount(app, "simulationShutdown") == (publicationFails ? 1 : 0));
    CHECK(eventCount(app, "shutdown") == (mainFails ? 0 : 1));
    const auto threads = app.threads();
    CHECK(std::none_of(threads.begin(), threads.end(), [](const auto& thread) {
        return thread.role != TechEngine::ThreadRole::PoolWorker;
    }));
    if (!mainFails) {
        CHECK(app.dedicatedJoinedBeforeShutdown);
    }
}

TEST_CASE("App wakes the main thread and shuts down after a simulation failure", "[app][lifecycle]") {
    AppLifecycleProbe app;
    bool mainEntered = false;
    app.onPublishSnapshot = [&](const TechEngine::SimulationContext& simulation) {
        if (simulation.tick > 0) {
            mainEntered = app.mainEntered.wait();
            throw std::runtime_error{"simulation publication failed"};
        }
    };

    CHECK(app.run() != 0);
    CHECK(mainEntered);
    CHECK(eventCount(app, "publishSnapshot") == 2);
    CHECK(eventCount(app, "simulationShutdown") == 1);
    CHECK(eventCount(app, "shutdown") == 1);
    checkStopped(app);
}

TEST_CASE("App finishes finite jobs before releasing simulation state", "[app][lifecycle]") {
    AppLifecycleProbe app;
    AppLifecycleSignal jobStarted;
    AppLifecycleSignal releaseJob;
    std::atomic<bool> jobReleased = false;
    std::atomic<bool> jobWaitCompleted = false;
    bool cleanupObservedCompletion = false;
    bool jobObserved = false;
    bool stoppedBeforeRelease = false;
    app.onFixedUpdate = [&](const TechEngine::SimulationContext& simulation) {
        std::array<TechEngine::Task, 1> tasks{[&] {
            jobStarted.set();
            jobReleased = releaseJob.wait();
        }};
        simulation.engine.jobs.wait(simulation.engine.jobs.submit(tasks));
        jobWaitCompleted = true;
    };
    app.onSimulationShutdown = [&] {
        cleanupObservedCompletion = jobWaitCompleted.load();
    };
    std::jthread controller{[&] {
        jobObserved = jobStarted.wait();
        app.requestStop();
        stoppedBeforeRelease = app.simulationStopped.wait(100ms);
        releaseJob.set();
    }};

    const int result = app.run();
    controller.join();
    CHECK(result == 0);
    CHECK(jobObserved);
    CHECK_FALSE(stoppedBeforeRelease);
    CHECK(jobReleased.load());
    CHECK(jobWaitCompleted.load());
    CHECK(cleanupObservedCompletion);
    checkStopped(app);
}

TEST_CASE("App cancels a waiting headless main thread without another input line", "[app][lifecycle]") {
    AppLifecycleProbe app;
    bool mainEntered = false;
    std::jthread controller{[&] {
        mainEntered = app.mainEntered.wait();
        app.requestStop();
        app.requestStop();
    }};

    const int result = app.run();
    controller.join();
    CHECK(result == 0);
    CHECK(mainEntered);
    CHECK(eventCount(app, "simulationShutdown") == 1);
    CHECK(eventCount(app, "shutdown") == 1);
    checkStopped(app);
}

TEST_CASE("App observes a stop requested before the main thread starts waiting", "[app][lifecycle]") {
    AppLifecycleProbe app;
    app.onSimulationInit = [&] {
        app.requestStop();
    };

    REQUIRE(app.run() == 0);
    CHECK(eventCount(app, "simulationInit") == 1);
    CHECK(eventCount(app, "fixedUpdate") == 0);
    CHECK(eventCount(app, "simulationShutdown") == 1);
    CHECK(eventCount(app, "shutdown") == 1);
    checkStopped(app);
}

TEST_CASE("App stop during main initialization skips simulation and cleans up the main", "[app][lifecycle]") {
    AppLifecycleProbe app;
    app.onInit = [&] {
        app.requestStop();
        app.requestStop();
    };

    REQUIRE(app.run() == 0);
    CHECK(eventCount(app, "init") == 1);
    CHECK(eventCount(app, "simulationInit") == 0);
    CHECK(eventCount(app, "mainUpdate") == 0);
    CHECK(eventCount(app, "simulationShutdown") == 0);
    CHECK(eventCount(app, "shutdown") == 1);
    CHECK(app.dedicatedJoinedBeforeShutdown);
}

TEST_CASE("App reports main shutdown failure and releases main registration", "[app][lifecycle]") {
    AppLifecycleProbe app;
    app.onInit = [&] {
        app.requestStop();
    };
    app.onShutdown = [] {
        throw std::runtime_error{"main shutdown failed"};
    };

    CHECK(app.run() != 0);
    CHECK(eventCount(app, "shutdown") == 1);
    const auto threads = app.threads();
    CHECK(std::none_of(threads.begin(), threads.end(), [](const auto& thread) {
        return thread.role != TechEngine::ThreadRole::PoolWorker;
    }));
}

TEST_CASE("App stops simulation when a main update hook throws", "[app][lifecycle]") {
    AppLifecycleProbe app;
    app.onMainUpdate = [] {
        throw std::runtime_error{"main update failed"};
    };

    CHECK(app.run() != 0);
    CHECK(eventCount(app, "mainUpdate") == 1);
    CHECK(eventCount(app, "simulationShutdown") == 1);
    CHECK(eventCount(app, "shutdown") == 1);
    checkStopped(app);
}

TEST_CASE("App reports simulation shutdown failures without replacing an earlier failure", "[app][lifecycle]") {
    AppLifecycleProbe app;
    bool publicationFails = false;
    bool nonStandardShutdownFailure = false;
    SECTION("shutdown fails after normal simulation work") {
    }
    SECTION("shutdown throws a non-standard exception") {
        nonStandardShutdownFailure = true;
    }
    SECTION("publication and shutdown both fail") {
        publicationFails = true;
    }

    app.onPublishSnapshot = [&](const TechEngine::SimulationContext& simulation) {
        if (simulation.tick == 0) {
            return;
        }
        if (publicationFails) {
            throw std::runtime_error{"original simulation failure"};
        }
        app.requestStop();
    };
    app.onSimulationShutdown = [&] {
        if (nonStandardShutdownFailure) {
            throw 42;
        }
        throw std::runtime_error{"simulation shutdown failed"};
    };

    CHECK(app.run() != 0);
    CHECK(eventCount(app, "simulationInit") == 1);
    CHECK(eventCount(app, "simulationShutdown") == 1);
    CHECK(eventCount(app, "shutdown") == 1);
    const auto completion = app.simulationCompletion();
    CHECK(completion.status == TechEngine::ThreadCompletionStatus::Failed);
    REQUIRE(completion.failure);
    if (publicationFails) {
        CHECK_THROWS_WITH(std::rethrow_exception(completion.failure), "original simulation failure");
    } else if (nonStandardShutdownFailure) {
        CHECK_THROWS_AS(std::rethrow_exception(completion.failure), int);
    } else {
        CHECK_THROWS_WITH(std::rethrow_exception(completion.failure), "simulation shutdown failed");
    }
    checkStopped(app);
}
