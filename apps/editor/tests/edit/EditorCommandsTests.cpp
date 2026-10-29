#include <TechEngine/app/App.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/testing/AppTestAccess.hpp>

#include <edit/EditSession.hpp>
#include <edit/EditorCommands.hpp>
#include <edit/EditorSnapshot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

using TechEngine::EditorCommands;
using TechEngine::EditorSnapshot;
using TechEngine::Entity;

static constexpr std::chrono::seconds COMMAND_TIMEOUT{5};

class CommandsAppProbe : public TechEngine::App {
private:
    std::mutex m_mutex;
    std::condition_variable m_published;
    EditorSnapshot m_latest;
    int m_publishes = 0;

public:
    // Touched by the commands on the simulation thread, and by the test only after run() returns.
    TechEngine::EditSession session;
    std::atomic<bool> startupLoaded = false;

    CommandsAppProbe() : App(TechEngine::Role::DedicatedServer) {
    }

    bool waitForPublishes(const int count, EditorSnapshot& out) {
        std::unique_lock lock{m_mutex};
        const bool reached = m_published.wait_for(lock, COMMAND_TIMEOUT, [this, count] {
            return m_publishes >= count;
        });
        out = m_latest;
        return reached;
    }

protected:
    void init() override {
    }

    bool simulationTicks() const override {
        return false;
    }

    std::error_code simulationStartupLoad(TechEngine::Scene&) override {
        startupLoaded = true;
        return {};
    }

    void publishEdits(const TechEngine::Scene& scene) override {
        EditorSnapshot snapshot;
        session.buildSnapshot(scene, snapshot);
        {
            const std::lock_guard lock{m_mutex};
            m_latest = std::move(snapshot);
            m_publishes++;
        }
        m_published.notify_all();
    }
};

static bool waitForStartup(const CommandsAppProbe& app) {
    const auto deadline = std::chrono::steady_clock::now() + COMMAND_TIMEOUT;
    while (!app.startupLoaded.load() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
    return app.startupLoaded.load();
}

static std::vector<std::string> g_editorCommandsLog;

static void captureEditorCommandsLog(const TechEngine::LogRecord& record) {
    g_editorCommandsLog.emplace_back(record.message);
}

class EditorCommandsLogCapture {
private:
    TechEngine::Level m_previousLevel;

public:
    EditorCommandsLogCapture() : m_previousLevel(TechEngine::minLevel()) {
        g_editorCommandsLog.clear();
        TechEngine::setMinLevel(TechEngine::Level::Trace);
        (void)TechEngine::addLogSink(&captureEditorCommandsLog);
    }

    ~EditorCommandsLogCapture() {
        (void)TechEngine::removeLogSink(&captureEditorCommandsLog);
        TechEngine::setMinLevel(m_previousLevel);
        g_editorCommandsLog.clear();
    }

    EditorCommandsLogCapture(const EditorCommandsLogCapture&) = delete;

    EditorCommandsLogCapture& operator=(const EditorCommandsLogCapture&) = delete;
};

TEST_CASE("every sender method reaches the live scene through the running simulation", "[editor][edit][commands][integration]") {
    CommandsAppProbe app;
    EditorCommands commands{app, app.session};
    bool startedInTime = false;
    std::vector<bool> stepsInTime;
    std::vector<EditorSnapshot> steps;

    std::jthread controller{[&] {
        startedInTime = waitForStartup(app);
        int expected = 0;
        const auto step = [&] {
            expected++;
            EditorSnapshot snapshot;
            stepsInTime.push_back(app.waitForPublishes(expected, snapshot));
            steps.push_back(snapshot);
            return snapshot;
        };
        if (startedInTime) {
            commands.create(Entity{});
            const EditorSnapshot created = step();
            const Entity parent = created.hierarchy.empty() ? Entity{} : created.hierarchy[0].entity;

            commands.create(parent);
            const EditorSnapshot withChild = step();
            const Entity child = withChild.hierarchy.size() < 2 ? Entity{} : withChild.hierarchy[1].entity;

            commands.rename(child, "Renamed");
            step();
            commands.select(child);
            step();
            commands.reparent(child, Entity{}, 0);
            step();
            commands.destroy(parent);
            step();
        }
        app.requestStop();
    }};
    REQUIRE(app.run() == 0);
    controller.join();

    REQUIRE(startedInTime);
    REQUIRE(steps.size() == 6);
    CHECK(std::ranges::all_of(stepsInTime, [](const bool inTime) {
        return inTime;
    }));

    REQUIRE(steps[0].hierarchy.size() == 1);
    const Entity parent = steps[0].hierarchy[0].entity;
    REQUIRE(steps[1].hierarchy.size() == 2);
    CHECK(steps[1].hierarchy[0].childCount == 1);
    const Entity child = steps[1].hierarchy[1].entity;
    CHECK(steps[1].hierarchy[1].depth == 1);
    REQUIRE(steps[2].hierarchy.size() == 2);
    CHECK(steps[2].hierarchy[1].name == "Renamed");
    CHECK(steps[3].selected == child);
    CHECK(steps[3].editRevision == steps[2].editRevision);
    CHECK(steps[4].hierarchy.size() == 2);
    CHECK(std::ranges::all_of(steps[4].hierarchy, [](const TechEngine::HierarchyRow& row) {
        return row.depth == 0;
    }));

    REQUIRE(steps[5].hierarchy.size() == 1);
    CHECK(steps[5].hierarchy[0].entity == child);
    CHECK(steps[5].hierarchy[0].name == "Renamed");
    CHECK(steps[5].selected == child);
    const TechEngine::Scene& scene = TechEngineTests::AppTestAccess::scene(app);
    CHECK_FALSE(scene.contains(parent));
    CHECK(scene.contains(child));
}

TEST_CASE("a sender on an app that is not running applies nothing and logs the refusal", "[editor][edit][commands]") {
    const EditorCommandsLogCapture logs;
    CommandsAppProbe app;
    EditorCommands commands{app, app.session};

    commands.create(Entity{});
    commands.destroy(Entity{0, 0});
    commands.reparent(Entity{0, 0}, Entity{}, 0);
    commands.rename(Entity{0, 0}, "Name");
    commands.select(Entity{0, 0});

    CHECK(TechEngineTests::AppTestAccess::scene(app).getRoots().empty());
    const auto refusals = std::ranges::count_if(g_editorCommandsLog, [](const std::string& message) {
        return message.find("refused an edit") != std::string::npos;
    });
    CHECK(refusals == 5);
}
