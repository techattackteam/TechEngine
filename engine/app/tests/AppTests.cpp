#include <TechEngine/app/App.hpp>
#include <TechEngine/app/Command.hpp>
#include <TechEngine/app/SimulationError.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/assets/AssetSource.hpp>
#include <TechEngine/core/jobs/JobSystem.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/serialization/BlobHeader.hpp>
#include <TechEngine/core/serialization/Writer.hpp>
#include <TechEngine/core/systems/ISystem.hpp>
#include <TechEngine/platform/files/FileError.hpp>
#include <TechEngine/testing/AppTestAccess.hpp>
#include <TechEngine/testing/AssertCapture.hpp>

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
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

        void configureSimulation() override {
            configurationCount++;
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
        int configurationCount = 0;
    };

    struct AppProbeEvent {
        static constexpr std::string_view tag = "Test.AppProbeEvent";

        std::uint32_t amount;
    };

    struct AppLateEvent {
        static constexpr std::string_view tag = "Test.AppLateEvent";

        std::uint32_t amount;
    };

    class EventProbeApp final : public TechEngine::App {
    public:
        EventProbeApp() : App(TechEngine::Role::DedicatedServer) {
        }

        void init() override {
        }

        void configureSimulation() override {
            configuredId = m_eventRegistry.registerEvent<AppProbeEvent>();
        }

        void finalize() {
            finalizeSimulation();
        }

        TechEngine::EventRegistry& eventRegistry() {
            return m_eventRegistry;
        }

        TechEngine::EventTypeId configuredId;
    };

    struct AppProbeResource {
        std::uint32_t value = 0;
    };

    constexpr TechEngine::BlobKind APP_PROBE_RESOURCE_KIND{0x50504152, 1};

    constexpr TechEngine::StringId APP_PROBE_RESOURCE_TYPE_TAG{"Test.AppProbeResource"};

    std::error_code loadAppProbeResource(std::span<const std::byte>, void*, std::vector<TechEngine::ResourceId>&) {
        return {};
    }

    class ResourceProbeApp : public TechEngine::App {
    public:
        ResourceProbeApp() : App(TechEngine::Role::DedicatedServer) {
        }

        void init() override {
        }

        void configureSimulation() override {
            m_resources.registerLoader<AppProbeResource>(APP_PROBE_RESOURCE_KIND, APP_PROBE_RESOURCE_TYPE_TAG, &loadAppProbeResource);
            closedDuringConfiguration = m_resources.loadersClosed();
        }

        void finalize() {
            finalizeSimulation();
        }

        TechEngine::ResourceCache& resources() {
            return m_resources;
        }

        bool closedDuringConfiguration = false;
    };

    class SceneReachProbe : public TechEngine::App {
    public:
        template<typename Probe = SceneReachProbe>
        static constexpr bool reachesScene() {
            return requires(Probe& probe) { probe.m_scene; };
        }

        template<typename Probe = SceneReachProbe>
        static constexpr bool reachesSchedule() {
            return requires(Probe& probe) { probe.m_schedule; };
        }
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
    CHECK(app.configurationCount == 1);
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

TEST_CASE("an App subclass cannot name App's scene", "[app]") {
    STATIC_CHECK(SceneReachProbe::reachesSchedule());
    STATIC_CHECK_FALSE(SceneReachProbe::reachesScene());
}

TEST_CASE("a mount added after construction is visible through App's context", "[app]") {
    ProbeApp app{TechEngine::Role::Client};

    app.table().mount("scratch", std::filesystem::temp_directory_path());

    std::filesystem::path resolved;
    const std::error_code resolveError = app.context().files.resolve("scratch://no-such-file", resolved);

    // NotFound means the context saw the mount and the file was absent, which is the point.
    // NoMount would mean EngineContext had snapshotted the table instead of referencing it,
    // and init() could then never mount anything the loop reads.
    REQUIRE(resolveError == TechEngine::FileError::NotFound);
}

TEST_CASE("App registers event types during configuration and closes registration when it finalizes", "[app]") {
    EventProbeApp app;

    REQUIRE_FALSE(app.eventRegistry().sealed());

    app.finalize();

    REQUIRE(app.configuredId.valid());
    REQUIRE(app.eventRegistry().typeCount() == 1);
    REQUIRE(app.eventRegistry().sealed());

    const TechEngineTests::AssertHandlerGuard guard;
    const TechEngine::EventTypeId late = app.eventRegistry().registerEvent<AppLateEvent>();

    REQUIRE_FALSE(late.valid());
    REQUIRE(app.eventRegistry().typeCount() == 1);
}

TEST_CASE("App closes resource loader registration when it finalizes", "[app][resources]") {
    ResourceProbeApp app;

    REQUIRE_FALSE(app.resources().loadersClosed());

    app.finalize();

    REQUIRE_FALSE(app.closedDuringConfiguration);
    REQUIRE(app.resources().loadersClosed());

    const TechEngineTests::FatalAssertGuard guard;
    REQUIRE_THROWS_AS(app.resources().registerLoader<AppProbeResource>(APP_PROBE_RESOURCE_KIND, APP_PROBE_RESOURCE_TYPE_TAG, &loadAppProbeResource), TechEngineTests::AssertFired);
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

static std::function<void(const TechEngine::SimulationContext&)> g_onLifecycleTick;
static const TechEngine::Scene* g_tickedScene = nullptr;

class AppLifecycleSystem final : public TechEngine::ISystem {
public:
    void init(TechEngine::ScheduleRegistration&) override {
    }

    void tick(TechEngine::Scene& scene, const TechEngine::SimulationContext& simulation) override {
        g_tickedScene = &scene;
        if (g_onLifecycleTick) {
            g_onLifecycleTick(simulation);
        }
    }

    std::string_view name() const override {
        return "AppLifecycleSystem";
    }
};

class AppLifecycleProbe : public TechEngine::App {
public:
    AppLifecycleProbe() : App(TechEngine::Role::DedicatedServer) {
    }

    ~AppLifecycleProbe() override {
        g_onLifecycleTick = {};
        g_tickedScene = nullptr;
    }

    std::function<void()> onInit;
    std::function<void()> onConfigureSimulation;
    std::function<void()> onMainUpdate;
    std::function<void()> onSimulationInit;
    std::function<std::error_code()> onSimulationStartupLoad;
    std::function<void(const TechEngine::SimulationContext&)> onSystemTick;
    std::function<void(const TechEngine::SimulationContext&)> onPublishSnapshot;
    std::function<void()> onSimulationShutdown;
    std::function<void()> onShutdown;
    AppLifecycleSignal mainEntered;
    AppLifecycleSignal simulationStopped;
    bool dedicatedJoinedBeforeShutdown = false;
    const TechEngine::Scene* loadedScene = nullptr;

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
        return m_clock.tick();
    }

    TechEngine::ThreadCompletionResult simulationCompletion() const {
        return m_simulationThread.completion();
    }

    std::error_code simulationStartupError() const {
        return m_simulationThread.startupError();
    }

protected:
    void init() override {
        record("init");
        if (onInit) {
            onInit();
        }
    }

    void configureSimulation() override {
        if (onConfigureSimulation) {
            onConfigureSimulation();
        }
        g_onLifecycleTick = [this](const TechEngine::SimulationContext& simulation) {
            record("systemTick", simulation.tick);
            if (onSystemTick) {
                onSystemTick(simulation);
            }
        };
        m_schedule.add<AppLifecycleSystem>();
    }

    void mainThreadUpdate() override {
        record("mainUpdate");
        mainEntered.set();
        if (onMainUpdate) {
            onMainUpdate();
        } else {
            App::mainThreadUpdate();
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

    std::error_code simulationStartupLoad(TechEngine::Scene& scene) override {
        record("simulationStartupLoad");
        loadedScene = &scene;
        if (onSimulationStartupLoad) {
            return onSimulationStartupLoad();
        }
        return {};
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

static void stopAtTick(AppLifecycleProbe& app, std::uint64_t tick) {
    app.onPublishSnapshot = [&app, tick](const TechEngine::SimulationContext& simulation) {
        if (simulation.tick >= tick) {
            app.requestStop();
        }
    };
}

static void checkOnlyPoolWorkersRegistered(const AppLifecycleProbe& app) {
    const auto threads = app.threads();
    CHECK(std::none_of(threads.begin(), threads.end(), [](const auto& thread) {
        return thread.role != TechEngine::ThreadRole::PoolWorker;
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
        return event.name == "systemTick" || event.name == "publishSnapshot";
    }));
    CHECK(app.dedicatedJoinedBeforeShutdown);
    checkOnlyPoolWorkersRegistered(app);
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
    stopAtTick(app, 3);

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
    CHECK(eventCount(app, "systemTick") >= 3);
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
        if (event.name == "systemTick") {
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
    stopAtTick(app, 5);

    REQUIRE(app.run() == 0);
    const TechEngine::TimingMetrics timing = app.timingMetrics();
    CHECK(timing.simulation.tick >= 5);
    CHECK(app.diagnosticFrame() == timing.simulation.tick);
    CHECK(eventCount(app, "systemTick") == timing.simulation.tick);
    CHECK_FALSE(timing.renderingActive);
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
    CHECK(eventCount(app, "simulationStartupLoad") == 0);
    CHECK(eventCount(app, "publishSnapshot") == (publicationFails ? 1 : 0));
    CHECK(eventCount(app, "systemTick") == 0);
    CHECK(eventCount(app, "simulationShutdown") == (publicationFails ? 1 : 0));
    CHECK(eventCount(app, "shutdown") == (mainFails ? 0 : 1));
    checkOnlyPoolWorkersRegistered(app);
    if (!mainFails) {
        CHECK(app.dedicatedJoinedBeforeShutdown);
    }
}

TEST_CASE("App shuts down initialized main state when simulation configuration fails", "[app][lifecycle]") {
    AppLifecycleProbe app;
    app.onConfigureSimulation = [] {
        throw std::runtime_error{"simulation configuration failed"};
    };

    CHECK(app.run() != 0);
    CHECK(eventCount(app, "init") == 1);
    CHECK(eventCount(app, "simulationInit") == 0);
    CHECK(eventCount(app, "systemTick") == 0);
    CHECK(eventCount(app, "publishSnapshot") == 0);
    CHECK(eventCount(app, "simulationShutdown") == 0);
    CHECK(eventCount(app, "shutdown") == 1);
    CHECK(app.dedicatedJoinedBeforeShutdown);
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

TEST_CASE("App runs the startup loading phase after readiness and before the first Tick", "[app][lifecycle][loading]") {
    AppLifecycleProbe app;
    bool mainEnteredDuringLoad = false;
    app.onSimulationStartupLoad = [&] {
        mainEnteredDuringLoad = app.mainEntered.wait();
        return std::error_code{};
    };
    stopAtTick(app, 1);

    REQUIRE(app.run() == 0);
    CHECK(mainEnteredDuringLoad);
    CHECK_FALSE(app.simulationStartupError());
    CHECK(eventCount(app, "simulationStartupLoad") == 1);

    std::vector<AppLifecycleEvent> simulationEvents;
    for (const auto& event: app.events()) {
        if (event.name == "simulationInit" || event.name == "publishSnapshot" || event.name == "simulationStartupLoad" || event.name == "systemTick") {
            simulationEvents.push_back(event);
        }
    }
    REQUIRE(simulationEvents.size() >= 4);
    CHECK(simulationEvents[0].name == "simulationInit");
    CHECK(simulationEvents[1].name == "publishSnapshot");
    CHECK(simulationEvents[1].tick == 0);
    CHECK(simulationEvents[2].name == "simulationStartupLoad");
    CHECK(simulationEvents[2].thread == simulationEvents[0].thread);
    CHECK(simulationEvents[3].name == "systemTick");
    CHECK(simulationEvents[3].tick == 1);
    checkStopped(app);
}

TEST_CASE("App hands the startup loading phase the scene its Ticks run on", "[app][lifecycle][loading]") {
    AppLifecycleProbe app;
    stopAtTick(app, 1);

    REQUIRE(app.run() == 0);
    REQUIRE(app.loadedScene != nullptr);
    CHECK(app.loadedScene == g_tickedScene);
    CHECK(app.loadedScene == &TechEngineTests::AppTestAccess::scene(app));
    checkStopped(app);
}

TEST_CASE("App restarts the timeline after a slow startup loading phase, so no catch-up Tick replays it", "[app][lifecycle][loading]") {
    const std::uint64_t clampedCatchUpTicks = 15;
    AppLifecycleProbe app;
    std::chrono::steady_clock::time_point loadFinished{};
    std::uint64_t initialTimeline = 0;
    std::uint64_t firstTickTimeline = 0;
    TechEngine::Clock::TimePoint firstTickTime{};
    std::uint64_t firstPublishedTick = 0;
    app.onSimulationStartupLoad = [&] {
        std::this_thread::sleep_for(400ms);
        loadFinished = std::chrono::steady_clock::now();
        return std::error_code{};
    };
    app.onSystemTick = [&](const TechEngine::SimulationContext& simulation) {
        if (simulation.tick == 1) {
            firstTickTime = simulation.tickTime;
            firstTickTimeline = simulation.timeline;
        }
    };
    app.onPublishSnapshot = [&](const TechEngine::SimulationContext& simulation) {
        if (simulation.tick == 0) {
            initialTimeline = simulation.timeline;
            return;
        }
        if (firstPublishedTick == 0) {
            firstPublishedTick = simulation.tick;
        }
        app.requestStop();
    };

    REQUIRE(app.run() == 0);
    CHECK(firstTickTime > loadFinished);
    CHECK(firstTickTimeline > initialTimeline);
    CHECK(firstPublishedTick >= 1);
    CHECK(firstPublishedTick < clampedCatchUpTicks);
    checkStopped(app);
}

TEST_CASE("App ends the run with the startup loading phase's error and runs no Tick", "[app][lifecycle][loading]") {
    AppLifecycleProbe app;
    const std::error_code loadError = TechEngine::FileError::NotFound;
    app.onSimulationStartupLoad = [&] {
        return loadError;
    };

    CHECK(app.run() != 0);
    CHECK(app.simulationStartupError() == loadError);
    CHECK(app.simulationCompletion().status == TechEngine::ThreadCompletionStatus::Completed);
    CHECK(eventCount(app, "simulationStartupLoad") == 1);
    CHECK(eventCount(app, "systemTick") == 0);
    CHECK(eventCount(app, "publishSnapshot") == 1);
    CHECK(eventCount(app, "simulationShutdown") == 1);
    CHECK(eventCount(app, "shutdown") == 1);
    checkStopped(app);
}

TEST_CASE("App stopped during the startup loading phase ends cleanly without a Tick", "[app][lifecycle][loading]") {
    AppLifecycleProbe app;
    app.onSimulationStartupLoad = [&] {
        app.requestStop();
        return std::error_code{};
    };

    CHECK(app.run() == 0);
    CHECK_FALSE(app.simulationStartupError());
    CHECK(eventCount(app, "simulationStartupLoad") == 1);
    CHECK(eventCount(app, "systemTick") == 0);
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
    app.onSystemTick = [&](const TechEngine::SimulationContext& simulation) {
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
    CHECK(eventCount(app, "systemTick") == 0);
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
    checkOnlyPoolWorkersRegistered(app);
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

struct AppCommandRecord {
    int id = 0;
    const TechEngine::Scene* scene = nullptr;
    std::size_t loadsBefore = 0;
    std::size_t ticksBefore = 0;
};

class AppCommandLog {
public:
    void applied(const AppCommandRecord& record) {
        std::lock_guard const lock{m_mutex};
        m_applied.push_back(record);
    }

    void destroyed() {
        std::lock_guard const lock{m_mutex};
        m_destroyed++;
    }

    std::vector<AppCommandRecord> appliedRecords() const {
        std::lock_guard const lock{m_mutex};
        return m_applied;
    }

    int destroyedCount() const {
        std::lock_guard const lock{m_mutex};
        return m_destroyed;
    }

private:
    mutable std::mutex m_mutex;
    std::vector<AppCommandRecord> m_applied;
    int m_destroyed = 0;
};

class AppProbeCommand final : public TechEngine::Command {
public:
    AppProbeCommand(const int id, AppCommandLog& log, const AppLifecycleProbe& app) : m_id(id), m_log(log), m_app(app) {
    }

    ~AppProbeCommand() override {
        m_log.destroyed();
    }

    void apply(TechEngine::Scene& scene) override {
        m_log.applied({.id = m_id, .scene = &scene, .loadsBefore = eventCount(m_app, "simulationStartupLoad"), .ticksBefore = eventCount(m_app, "systemTick")});
    }

private:
    int m_id;
    AppCommandLog& m_log;
    const AppLifecycleProbe& m_app;
};

static std::error_code submitProbe(AppLifecycleProbe& app, const int id, AppCommandLog& log) {
    return app.submitCommand(std::make_unique<AppProbeCommand>(id, log, app));
}

static std::mutex g_appWarningsMutex;
static std::vector<std::string> g_appWarnings;

static void captureAppWarning(const TechEngine::LogRecord& record) {
    if (record.level != TechEngine::Level::Warn) {
        return;
    }
    std::lock_guard const lock{g_appWarningsMutex};
    g_appWarnings.emplace_back(record.message);
}

class AppWarningCapture {
public:
    AppWarningCapture() : m_previousLevel(TechEngine::minLevel()) {
        clear();
        TechEngine::setMinLevel(TechEngine::Level::Warn);
        (void)TechEngine::addLogSink(&captureAppWarning);
    }

    ~AppWarningCapture() {
        (void)TechEngine::removeLogSink(&captureAppWarning);
        TechEngine::setMinLevel(m_previousLevel);
        clear();
    }

    AppWarningCapture(const AppWarningCapture&) = delete;

    AppWarningCapture& operator=(const AppWarningCapture&) = delete;

    std::vector<std::string> containing(std::string_view text) const {
        std::lock_guard const lock{g_appWarningsMutex};
        std::vector<std::string> matches;
        for (const std::string& warning: g_appWarnings) {
            const auto found = std::ranges::search(warning, text, [](const char left, const char right) {
                return std::tolower(static_cast<unsigned char>(left)) == std::tolower(static_cast<unsigned char>(right));
            });
            if (!found.empty()) {
                matches.push_back(warning);
            }
        }
        return matches;
    }

private:
    static void clear() {
        std::lock_guard const lock{g_appWarningsMutex};
        g_appWarnings.clear();
    }

    TechEngine::Level m_previousLevel;
};

TEST_CASE("App applies submitted commands to the live scene in FIFO order before that loop pass's ticks", "[app][lifecycle][commands]") {
    const AppWarningCapture warnings;
    AppLifecycleProbe app;
    AppCommandLog log;
    std::vector<std::error_code> submitErrors;
    app.onSimulationStartupLoad = [&] {
        for (int id = 1; id <= 3; id++) {
            submitErrors.push_back(submitProbe(app, id, log));
        }
        return std::error_code{};
    };
    stopAtTick(app, 2);

    REQUIRE(app.run() == 0);
    REQUIRE(submitErrors.size() == 3);
    for (const std::error_code& submitError: submitErrors) {
        CHECK_FALSE(submitError);
    }
    const std::vector<AppCommandRecord> applied = log.appliedRecords();
    REQUIRE(applied.size() == 3);
    for (std::size_t i = 0; i < applied.size(); i++) {
        CHECK(applied[i].id == static_cast<int>(i) + 1);
        CHECK(applied[i].scene == &TechEngineTests::AppTestAccess::scene(app));
        CHECK(applied[i].loadsBefore == 1);
        CHECK(applied[i].ticksBefore == 0);
    }
    CHECK(log.destroyedCount() == 3);
    CHECK(warnings.containing("cancel").empty());
    checkStopped(app);
}

TEST_CASE("App rejects and never applies a command submitted before the run or after stop is requested", "[app][lifecycle][commands]") {
    AppLifecycleProbe app;
    AppCommandLog log;
    const std::error_code beforeRunError = submitProbe(app, 0, log);
    std::error_code duringStopError;
    app.onSimulationStartupLoad = [&] {
        app.requestStop();
        duringStopError = submitProbe(app, 1, log);
        return std::error_code{};
    };

    REQUIRE(app.run() == 0);
    const std::error_code afterRunError = submitProbe(app, 2, log);

    CHECK(beforeRunError == TechEngine::SimulationError::CommandQueueClosed);
    CHECK(duringStopError == TechEngine::SimulationError::CommandQueueClosed);
    CHECK(afterRunError == TechEngine::SimulationError::CommandQueueClosed);
    CHECK(log.appliedRecords().empty());
    CHECK(log.destroyedCount() == 3);
    checkStopped(app);
}

TEST_CASE("App accepts commands again on its next run", "[app][lifecycle][commands]") {
    AppLifecycleProbe app;
    AppCommandLog log;
    app.onSimulationStartupLoad = [&] {
        app.requestStop();
        return std::error_code{};
    };
    REQUIRE(app.run() == 0);

    std::error_code secondRunError = TechEngine::SimulationError::CommandQueueClosed;
    app.onSimulationStartupLoad = [&] {
        secondRunError = submitProbe(app, 1, log);
        return std::error_code{};
    };
    stopAtTick(app, 1);

    REQUIRE(app.run() == 0);
    CHECK_FALSE(secondRunError);
    const std::vector<AppCommandRecord> applied = log.appliedRecords();
    REQUIRE(applied.size() == 1);
    CHECK(applied.front().id == 1);
}

TEST_CASE("App reports commands accepted but not applied at shutdown as cancelled", "[app][lifecycle][commands]") {
    const AppWarningCapture warnings;
    AppLifecycleProbe app;
    AppCommandLog log;
    std::vector<std::error_code> submitErrors;
    bool loadFails = false;
    SECTION("stop is requested during the startup load") {
    }
    SECTION("the startup load fails") {
        loadFails = true;
    }
    app.onSimulationStartupLoad = [&]() -> std::error_code {
        for (int id = 1; id <= 2; id++) {
            submitErrors.push_back(submitProbe(app, id, log));
        }
        if (loadFails) {
            return TechEngine::FileError::NotFound;
        }
        app.requestStop();
        return {};
    };

    const int result = app.run();

    CHECK(result == (loadFails ? 1 : 0));
    REQUIRE(submitErrors.size() == 2);
    for (const std::error_code& submitError: submitErrors) {
        CHECK_FALSE(submitError);
    }
    CHECK(log.appliedRecords().empty());
    CHECK(log.destroyedCount() == 2);
    const std::vector<std::string> cancelled = warnings.containing("cancel");
    REQUIRE(cancelled.size() == 1);
    CHECK(cancelled.front().find('2') != std::string::npos);
    checkStopped(app);
}

constexpr TechEngine::ResourceId APP_HELD_RESOURCE_ID{0xA001};

class AppHeldResourceSource final : public TechEngine::AssetSource {
public:
    TechEngine::AssetAvailability find(const TechEngine::ResourceId id) const override {
        return id == APP_HELD_RESOURCE_ID ? TechEngine::AssetAvailability::Present : TechEngine::AssetAvailability::Unknown;
    }

    std::error_code read(const TechEngine::ResourceId id, std::vector<std::byte>& out) const override {
        if (id != APP_HELD_RESOURCE_ID) {
            return TechEngine::FileError::NotFound;
        }
        out.clear();
        TechEngine::Writer writer{out};
        writer.writeHeader(APP_PROBE_RESOURCE_KIND);
        return {};
    }
};

class AppHeldResourceProbe final : public TechEngine::App {
public:
    AppHeldResourceProbe(const AppWarningCapture& warnings, const bool releaseOnShutdown) : App(TechEngine::Role::DedicatedServer), m_warnings(warnings), m_releaseOnShutdown(releaseOnShutdown) {
    }

    TechEngine::ResourceState loadedState = TechEngine::ResourceState::Pending;
    std::size_t warningsAtSimulationShutdown = 0;

protected:
    void init() override {
    }

    void configureSimulation() override {
        m_resources.setSource(m_source);
        m_resources.registerLoader<AppProbeResource>(APP_PROBE_RESOURCE_KIND, APP_PROBE_RESOURCE_TYPE_TAG, &loadAppProbeResource);
    }

    std::error_code simulationStartupLoad(TechEngine::Scene&) override {
        const std::error_code requestError = m_resources.request(APP_HELD_RESOURCE_ID, m_held);
        if (requestError) {
            return requestError;
        }
        const std::error_code loadError = m_resources.runLoadingPhase();
        m_resources.getState(m_held, loadedState);
        requestStop();
        return loadError;
    }

    void simulationShutdown() override {
        warningsAtSimulationShutdown = m_warnings.containing("not released").size();
        if (m_releaseOnShutdown) {
            m_resources.release(m_held);
            m_held = {};
        }
        App::simulationShutdown();
    }

private:
    const AppWarningCapture& m_warnings;
    bool m_releaseOnShutdown;
    AppHeldResourceSource m_source;
    TechEngine::ResourceHandle<AppProbeResource> m_held;
};

TEST_CASE("App's releases in simulation shutdown land before the cache logs what is still counted", "[app][lifecycle][resources]") {
    const AppWarningCapture warnings;
    bool releaseOnShutdown = true;
    SECTION("the app releases its resource in simulation shutdown") {
    }
    SECTION("the app keeps its resource past simulation shutdown") {
        releaseOnShutdown = false;
    }
    AppHeldResourceProbe app{warnings, releaseOnShutdown};

    REQUIRE(app.run() == 0);
    CHECK(app.loadedState == TechEngine::ResourceState::Ready);
    CHECK(app.warningsAtSimulationShutdown == 0);
    const std::vector<std::string> leaks = warnings.containing("not released");
    if (releaseOnShutdown) {
        CHECK(leaks.empty());
    } else {
        REQUIRE(leaks.size() == 1);
        CHECK(leaks.front().find("a001") != std::string::npos);
    }
}
