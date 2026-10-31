#include <TechEngine/app/App.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/testing/AppTestAccess.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <edit/EditSession.hpp>
#include <edit/EditorCommands.hpp>
#include <edit/EditorSnapshot.hpp>
#include <edit/FieldValue.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

using TechEngine::EditorCommands;
using TechEngine::EditorSnapshot;
using TechEngine::Entity;
using TechEngine::FieldValue;

static constexpr std::chrono::seconds COMMAND_TIMEOUT{5};

struct CommandsProbeComponent {
    static constexpr std::string_view tag = "Tests.CommandsProbeComponent";

    std::uint32_t value = 0;

    template<typename Archive>
    friend void visit(Archive& archive, CommandsProbeComponent& component) {
        archive.field("value", component.value);
    }
};

static const TechEngine::ComponentRow* findProbeRow(const EditorSnapshot& snapshot, const TechEngine::ComponentTypeId type) {
    const auto row = std::ranges::find(snapshot.components, type, &TechEngine::ComponentRow::type);
    if (row == snapshot.components.end()) {
        return nullptr;
    }
    return &*row;
}

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

    const TechEngine::ComponentRegistry& getRegistry() const {
        return m_registry;
    }

    TechEngine::FileAccess& getFiles() {
        return m_files;
    }

    void mountProject(const std::filesystem::path& root) {
        m_mounts.mount("project", root);
    }

    bool isStopRequested() const {
        return stopRequested();
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

    void configureSimulation() override {
        m_registry.registerComponent<CommandsProbeComponent>(CommandsProbeComponent::tag);
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
        session.buildSnapshot(scene, m_registry, snapshot);
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
    EditorCommands commands{app, app.session, app.getRegistry(), app.getFiles()};
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
            commands.setField(child, TechEngine::componentTypeId<TechEngine::Transform>(), 0, FieldValue{2.0f});
            step();
            commands.addComponent(child, TechEngine::componentTypeId<CommandsProbeComponent>());
            step();
            commands.removeComponent(child, TechEngine::componentTypeId<CommandsProbeComponent>());
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
    REQUIRE(steps.size() == 9);
    CHECK(std::ranges::all_of(stepsInTime, [](const bool inTime) {
        return inTime;
    }));

    const TechEngine::ComponentTypeId transformType = TechEngine::componentTypeId<TechEngine::Transform>();
    const TechEngine::ComponentTypeId probeType = TechEngine::componentTypeId<CommandsProbeComponent>();
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
    CHECK(findProbeRow(steps[3], transformType) != nullptr);

    const TechEngine::ComponentRow* edited = findProbeRow(steps[4], transformType);
    REQUIRE(edited != nullptr);
    REQUIRE_FALSE(edited->fields.empty());
    CHECK(edited->fields[0].value == FieldValue{2.0f});
    CHECK(steps[4].editRevision > steps[3].editRevision);
    CHECK(findProbeRow(steps[5], probeType) != nullptr);
    CHECK(findProbeRow(steps[6], probeType) == nullptr);

    CHECK(steps[7].hierarchy.size() == 2);
    CHECK(std::ranges::all_of(steps[7].hierarchy, [](const TechEngine::HierarchyRow& row) {
        return row.depth == 0;
    }));

    REQUIRE(steps[8].hierarchy.size() == 1);
    CHECK(steps[8].hierarchy[0].entity == child);
    CHECK(steps[8].hierarchy[0].name == "Renamed");
    CHECK(steps[8].selected == child);
    const TechEngine::Scene& scene = TechEngineTests::AppTestAccess::scene(app);
    CHECK_FALSE(scene.contains(parent));
    CHECK(scene.contains(child));
}

TEST_CASE("a sender on an app that is not running applies nothing and logs the refusal", "[editor][edit][commands]") {
    const EditorCommandsLogCapture logs;
    CommandsAppProbe app;
    EditorCommands commands{app, app.session, app.getRegistry(), app.getFiles()};

    commands.create(Entity{});
    commands.destroy(Entity{0, 0});
    commands.reparent(Entity{0, 0}, Entity{}, 0);
    commands.rename(Entity{0, 0}, "Name");
    commands.select(Entity{0, 0});
    commands.setField(Entity{0, 0}, TechEngine::ComponentTypeId{}, 0, FieldValue{1.0f});
    commands.addComponent(Entity{0, 0}, TechEngine::ComponentTypeId{});
    commands.removeComponent(Entity{0, 0}, TechEngine::ComponentTypeId{});
    commands.save();
    commands.saveAndClose();

    CHECK(TechEngineTests::AppTestAccess::scene(app).getRoots().empty());
    const auto refusals = std::ranges::count_if(g_editorCommandsLog, [](const std::string& message) {
        return message.find("refused an edit") != std::string::npos;
    });
    CHECK(refusals == 10);
}

TEST_CASE("save and save and close reach the live scene through the running simulation", "[editor][edit][commands][save][integration]") {
    const TechEngineTests::ScratchDirectory scratch{"editorCommandsSave"};
    constexpr TechEngine::ResourceId sceneId{0x5a5a5a5a00000002};
    constexpr std::string_view scenePath = "project://Saved.tescene";
    CommandsAppProbe app;
    app.mountProject(scratch.root());
    app.session.setSceneFile({sceneId, std::string(scenePath)});
    EditorCommands commands{app, app.session, app.getRegistry(), app.getFiles()};
    bool startedInTime = false;
    std::vector<bool> stepsInTime;
    std::vector<EditorSnapshot> steps;
    bool stoppedAfterSave = true;
    bool stoppedAfterSaveAndClose = false;

    std::jthread controller{[&] {
        startedInTime = waitForStartup(app);
        int expected = 0;
        const auto step = [&] {
            expected++;
            EditorSnapshot snapshot;
            stepsInTime.push_back(app.waitForPublishes(expected, snapshot));
            steps.push_back(snapshot);
        };
        if (startedInTime) {
            commands.create(Entity{});
            step();
            commands.save();
            step();
            stoppedAfterSave = app.isStopRequested();
            commands.create(Entity{});
            step();
            commands.saveAndClose();
            step();
            stoppedAfterSaveAndClose = app.isStopRequested();
        }
        app.requestStop();
    }};
    REQUIRE(app.run() == 0);
    controller.join();

    REQUIRE(startedInTime);
    REQUIRE(steps.size() == 4);
    CHECK(std::ranges::all_of(stepsInTime, [](const bool inTime) {
        return inTime;
    }));
    CHECK(steps[0].hasUnsavedEdits());
    CHECK_FALSE(steps[1].hasUnsavedEdits());
    CHECK(steps[1].savedRevision == steps[0].editRevision);
    CHECK_FALSE(stoppedAfterSave);
    CHECK(steps[2].hasUnsavedEdits());
    CHECK_FALSE(steps[3].hasUnsavedEdits());
    CHECK(stoppedAfterSaveAndClose);

    std::vector<std::byte> bytes;
    REQUIRE(app.getFiles().read(scenePath, bytes) == std::error_code{});
    std::vector<std::byte> expectedBytes;
    TechEngine::SceneAsset{sceneId}.write(TechEngineTests::AppTestAccess::scene(app), expectedBytes);
    CHECK(bytes == expectedBytes);
    CHECK(TechEngineTests::AppTestAccess::scene(app).getRoots().size() == 2);
}
