#include <TechEngine/core/resources/PackReader.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/MountTable.hpp>
#include <TechEngine/testing/AssertCapture.hpp>
#include <TechEngine/testing/ScratchDirectory.hpp>

#include <EditorApp.hpp>
#include <assets/AssetIndex.hpp>
#include <assets/AssetTestFiles.hpp>
#include <project/Project.hpp>

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <filesystem>
#include <iterator>
#include <string_view>
#include <system_error>
#include <thread>
#include <vector>

using TechEngineTests::ScratchDirectory;

namespace {
    class EditorProbe : public TechEngine::EditorApp {
    public:
        using TechEngine::EditorApp::EditorApp;
        using TechEngine::EditorApp::shouldClose;
        using TechEngine::EditorApp::shutdown;

        void advanceFrame(double deltaTime) {
            m_simulationThread.advance(deltaTime, [this](const TechEngine::SimulationContext& frame) {
                executeSimulationTick(frame);
            });
            publishSnapshot(m_simulationThread.simulationContext());
        }

        double ticksPerSecond() const {
            return m_simulationThread.timing().ticksPerSecond;
        }

        TechEngine::Role loopRole() const {
            return m_simulationThread.simulationContext().role;
        }

        void prepare() {
            init();
            finalizeSimulation();
        }

        std::error_code startupLoad() {
            return simulationStartupLoad();
        }

        void bootstrap() {
            prepare();
            const std::error_code loadError = startupLoad();
            REQUIRE(loadError == std::error_code{});
        }

        const TechEngine::Scene& scene() const {
            return m_scene;
        }

        const TechEngine::MountTable& mounts() const {
            return m_mounts;
        }

        const TechEngine::FileAccess& files() const {
            return m_files;
        }
    };

    constexpr std::string_view DEV_MANIFEST = "name = \"Dev\"\n";
    constexpr std::string_view STARTUP_SCENE_FILE = "assets/common/Startup.tescene";
    constexpr std::string_view STARTUP_SCENE_PATH = "project://assets/common/Startup.tescene";
    constexpr std::string_view MANIFEST_PATH = "project://project.toml";

    void writeProjectLayout(const ScratchDirectory& scratch) {
        scratch.writeFile("project.toml", DEV_MANIFEST);
        scratch.makeDirectory("assets/common");
        scratch.makeDirectory("assets/client");
        scratch.makeDirectory("assets/server");
    }

    TechEngine::ResourceId savedStartupScene(const ScratchDirectory& scratch) {
        TechEngine::MountTable mounts;
        mounts.mount("project", scratch.root());
        const TechEngine::FileAccess files{mounts};
        TechEngine::Project project;
        REQUIRE(project.load(files, MANIFEST_PATH) == std::error_code{});
        CHECK(project.name() == "Dev");
        return project.getExportStartupScene(TechEngine::Role::Client);
    }

    std::ptrdiff_t fileCount(const std::filesystem::path& directory) {
        return std::distance(std::filesystem::directory_iterator{directory}, std::filesystem::directory_iterator{});
    }
}

TEST_CASE("editor hosts a client", "[editor]") {
    const EditorProbe editor{std::filesystem::path{}, std::filesystem::path{}};

    REQUIRE(editor.loopRole() == TechEngine::Role::Client);
}

TEST_CASE("init mounts the project set", "[editor]") {
    ScratchDirectory scratch{"editorBootstrap"};
    writeProjectLayout(scratch);

    EditorProbe editor{scratch.root(), scratch.makeDirectory("export")};
    editor.bootstrap();

    CHECK(editor.mounts().hasAlias("project"));
    CHECK(editor.mounts().hasAlias("engine"));
    CHECK(editor.mounts().hasAlias("assets"));
    CHECK_FALSE(editor.mounts().hasAlias("shaders"));
    CHECK(editor.mounts().mountCount() == 4);
}

TEST_CASE("init indexes the project's assets", "[editor]") {
    ScratchDirectory scratch{"editorIndexesAssets"};
    writeProjectLayout(scratch);
    constexpr TechEngine::ResourceId sceneId{0x0123456789abcdef};
    TechEngineTests::writeBytes(scratch, "assets/common/level.tescene", TechEngineTests::assetBytes(TechEngine::SCENE_KIND, sceneId, "scene"));

    EditorProbe editor{scratch.root(), scratch.makeDirectory("export")};
    editor.bootstrap();

    TechEngine::AssetRecord record;
    REQUIRE(editor.getAssetIndex().find(sceneId, record));
    CHECK(record.side == TechEngine::Side::Common);
    CHECK(record.path == "project://assets/common/level.tescene");
}

TEST_CASE("the editor keeps running when the startup export fails", "[editor][window]") {
    ScratchDirectory scratch{"editorStartupExportFails"};
    writeProjectLayout(scratch);
    scratch.writeFile("project.toml", "name = \"Dev\"\n\n[export.client]\nstartupScene = \"0x00000000000000aa\"\n");
    const std::filesystem::path exportFolder = scratch.makeDirectory("export");

    EditorProbe editor{scratch.root(), exportFolder};
    editor.bootstrap();

    CHECK_FALSE(std::filesystem::exists(scratch.root() / STARTUP_SCENE_FILE));
    CHECK_FALSE(std::filesystem::exists(exportFolder / "Dev"));
    CHECK_FALSE(editor.shouldClose());
    editor.shutdown();
}

TEST_CASE("the startup load creates a startup scene when the project names none", "[editor][window]") {
    ScratchDirectory scratch{"editorCreatesStartupScene"};
    writeProjectLayout(scratch);
    const std::filesystem::path exportFolder = scratch.makeDirectory("export");

    EditorProbe editor{scratch.root(), exportFolder};
    editor.bootstrap();

    SECTION("it saves the scene's id as the client startup scene") {
        const TechEngine::ResourceId id = savedStartupScene(scratch);
        CHECK(id.valid());
    }

    SECTION("it writes a parent with two children to assets/common through the scene writer") {
        const TechEngine::ResourceId id = savedStartupScene(scratch);
        std::vector<std::byte> bytes;
        REQUIRE(editor.files().read(STARTUP_SCENE_PATH, bytes) == std::error_code{});

        TechEngine::ComponentRegistry registry;
        registry.registerComponent<TechEngine::Hierarchy>(TechEngine::Hierarchy::tag);
        registry.registerComponent<TechEngine::Transform>(TechEngine::Transform::tag);
        TechEngine::Scene scene(registry);
        TechEngine::SceneAsset asset;
        REQUIRE(asset.read(bytes, scene) == std::error_code{});
        CHECK(asset.id == id);

        const std::vector<TechEngine::Entity> roots = scene.getRoots();
        REQUIRE(roots.size() == 1);
        const std::vector<TechEngine::Entity> children = scene.getChildren(roots[0]);
        REQUIRE(children.size() == 2);
        CHECK(scene.getChildren(children[0]).empty());
        CHECK(scene.getChildren(children[1]).empty());
        CHECK(fileCount(scratch.root() / "assets" / "common") == 1);
    }

    SECTION("it leaves the scene in the editor's live scene") {
        const std::vector<TechEngine::Entity> roots = editor.scene().getRoots();
        REQUIRE(roots.size() == 1);
        const std::vector<TechEngine::Entity> children = editor.scene().getChildren(roots[0]);
        REQUIRE(children.size() == 2);
        CHECK(editor.scene().getChildren(children[0]).empty());
        CHECK(editor.scene().getChildren(children[1]).empty());
    }

    SECTION("it adds the scene to the id index") {
        const TechEngine::ResourceId id = savedStartupScene(scratch);
        TechEngine::AssetRecord record;
        REQUIRE(editor.getAssetIndex().find(id, record));
        CHECK(record.typeTag == TechEngine::SCENE_TYPE_TAG);
        CHECK(record.side == TechEngine::Side::Common);
        CHECK(record.path == STARTUP_SCENE_PATH);
        CHECK(editor.getAssetIndex().getRecords().size() == 1);
    }

    SECTION("the startup export packs it into the export folder") {
        const TechEngine::ResourceId id = savedStartupScene(scratch);
        TechEngine::MountTable exportMounts;
        exportMounts.mount("export", exportFolder);
        const TechEngine::FileAccess exportFiles{exportMounts};
        TechEngine::PackReader common{exportFiles};
        REQUIRE(common.open("export://Dev/assets/common.tepack") == std::error_code{});

        TechEngine::PackEntry entry;
        REQUIRE(common.find(id, entry));
        std::vector<std::byte> packed;
        REQUIRE(common.read(entry, packed) == std::error_code{});
        std::vector<std::byte> onDisk;
        REQUIRE(editor.files().read(STARTUP_SCENE_PATH, onDisk) == std::error_code{});
        CHECK(packed == onDisk);
        CHECK(std::filesystem::exists(exportFolder / "Dev" / "assets" / "client.tepack"));
    }

    editor.shutdown();
}

TEST_CASE("a later launch finds the saved startup scene and creates nothing", "[editor][window]") {
    ScratchDirectory scratch{"editorReusesStartupScene"};
    writeProjectLayout(scratch);
    const std::filesystem::path exportFolder = scratch.makeDirectory("export");

    std::vector<std::byte> firstScene;
    std::vector<std::byte> firstManifest;
    {
        EditorProbe first{scratch.root(), exportFolder};
        first.bootstrap();
        REQUIRE(first.files().read(STARTUP_SCENE_PATH, firstScene) == std::error_code{});
        REQUIRE(first.files().read(MANIFEST_PATH, firstManifest) == std::error_code{});
        first.shutdown();
    }
    const TechEngine::ResourceId firstId = savedStartupScene(scratch);
    REQUIRE(firstId.valid());

    EditorProbe second{scratch.root(), exportFolder};
    second.bootstrap();

    std::vector<std::byte> secondScene;
    std::vector<std::byte> secondManifest;
    REQUIRE(second.files().read(STARTUP_SCENE_PATH, secondScene) == std::error_code{});
    REQUIRE(second.files().read(MANIFEST_PATH, secondManifest) == std::error_code{});
    CHECK(secondScene == firstScene);
    CHECK(secondManifest == firstManifest);
    CHECK(savedStartupScene(scratch) == firstId);
    CHECK(second.getAssetIndex().getRecords().size() == 1);
    CHECK(fileCount(scratch.root() / "assets" / "common") == 1);
    second.shutdown();
}

TEST_CASE("the startup load creates nothing when the project already names a startup scene", "[editor][window]") {
    ScratchDirectory scratch{"editorKeepsNamedStartupScene"};
    writeProjectLayout(scratch);
    constexpr TechEngine::ResourceId sceneId{0x0123456789abcdef};
    scratch.writeFile("project.toml", "name = \"Dev\"\n\n[export.client]\nstartupScene = \"0x0123456789abcdef\"\n");
    TechEngineTests::writeBytes(scratch, "assets/common/level.tescene", TechEngineTests::assetBytes(TechEngine::SCENE_KIND, sceneId, "scene"));

    EditorProbe editor{scratch.root(), scratch.makeDirectory("export")};
    editor.bootstrap();

    CHECK_FALSE(std::filesystem::exists(scratch.root() / STARTUP_SCENE_FILE));
    CHECK(savedStartupScene(scratch) == sceneId);
    CHECK(editor.getAssetIndex().getRecords().size() == 1);
    editor.shutdown();
}

TEST_CASE("the startup load adopts an indexed Startup.tescene when the project names no startup scene", "[editor][window]") {
    ScratchDirectory scratch{"editorAdoptsStartupScene"};
    writeProjectLayout(scratch);
    constexpr TechEngine::ResourceId sceneId{0x0fedcba987654321};
    const std::vector<std::byte> original = TechEngineTests::assetBytes(TechEngine::SCENE_KIND, sceneId, "scene");
    TechEngineTests::writeBytes(scratch, STARTUP_SCENE_FILE, original);

    EditorProbe editor{scratch.root(), scratch.makeDirectory("export")};
    editor.bootstrap();

    std::vector<std::byte> after;
    REQUIRE(editor.files().read(STARTUP_SCENE_PATH, after) == std::error_code{});
    CHECK(after == original);
    CHECK(savedStartupScene(scratch) == sceneId);
    CHECK(editor.getAssetIndex().getRecords().size() == 1);
    CHECK(editor.scene().getRoots().empty());
    editor.shutdown();
}

TEST_CASE("the startup load does not overwrite a Startup.tescene the index skipped", "[editor][window]") {
    ScratchDirectory scratch{"editorKeepsUnindexedStartupScene"};
    writeProjectLayout(scratch);
    const std::vector<std::byte> original = TechEngineTests::assetBytes(TechEngine::SCENE_KIND, TechEngine::ResourceId(), "scene");
    TechEngineTests::writeBytes(scratch, STARTUP_SCENE_FILE, original);

    EditorProbe editor{scratch.root(), scratch.makeDirectory("export")};
    editor.prepare();
    {
        const TechEngineTests::FatalAssertGuard guard;
        REQUIRE_THROWS_AS(editor.startupLoad(), TechEngineTests::AssertFired);
    }

    std::vector<std::byte> after;
    REQUIRE(editor.files().read(STARTUP_SCENE_PATH, after) == std::error_code{});
    CHECK(after == original);
    CHECK_FALSE(savedStartupScene(scratch).valid());
    CHECK(editor.getAssetIndex().getRecords().empty());
    CHECK(editor.scene().getRoots().empty());
    editor.shutdown();
}

TEST_CASE("assets/client shadows assets/common", "[editor]") {
    ScratchDirectory scratch{"editorAssetsOverlay"};
    writeProjectLayout(scratch);
    scratch.writeFile("assets/common/shared.txt", "common");
    const std::filesystem::path client = scratch.writeFile("assets/client/shared.txt", "client");

    EditorProbe editor{scratch.root(), scratch.makeDirectory("export")};
    editor.bootstrap();

    std::filesystem::path resolved;
    REQUIRE(editor.files().resolve("assets://shared.txt", resolved) == std::error_code{});
    CHECK(resolved == client);
}

TEST_CASE("editor updates through a rate sample and shuts down its client", "[editor][window]") {
    ScratchDirectory scratch{"editorLifecycle"};
    writeProjectLayout(scratch);
    EditorProbe editor{scratch.root(), scratch.makeDirectory("export")};
    CHECK(editor.shouldClose());
    editor.bootstrap();
    REQUIRE_FALSE(editor.shouldClose());

    for (int i = 0; i < 8; i++) {
        editor.advanceFrame(0.125);
        CHECK(editor.ticksPerSecond() == (i == 7 ? 60.0 : 0.0));
        CHECK_FALSE(editor.shouldClose());
    }

    editor.shutdown();
    CHECK(editor.shouldClose());
    editor.shutdown();
}

class EditorStallProbe : public TechEngine::EditorApp {
public:
    using EditorApp::EditorApp;
    std::atomic<bool> mainEntered = false;
    std::atomic<bool> releaseMain = false;
    std::atomic<bool> inputConsumed = false;
    std::atomic<bool> mainTimedOut = false;

protected:
    void mainThreadUpdate() override {
        mainEntered.store(true);
        const auto deadline = m_clock.now() + std::chrono::seconds{5};
        while (!releaseMain.load() && m_clock.now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        if (!releaseMain.load()) {
            mainTimedOut = true;
            requestStop();
            return;
        }
        m_input.publish(TechEngine::InputEvent{.kind = TechEngine::InputKind::Focus, .pressed = true});
        m_input.publish(TechEngine::InputEvent{.kind = TechEngine::InputKind::Key, .key = TechEngine::Key::W, .pressed = true});
        EditorApp::mainThreadUpdate();
    }

    void publishSnapshot(const TechEngine::SimulationContext& simulation) override {
        EditorApp::publishSnapshot(simulation);
        if (simulation.input.held.isHeld(TechEngine::Key::W)) {
            inputConsumed = true;
            requestStop();
        }
    }
};

TEST_CASE("editor simulation and render progress during a main stall then consume delayed input", "[editor][window][integration]") {
    ScratchDirectory scratch{"editorThreadedStall"};
    writeProjectLayout(scratch);
    EditorStallProbe editor{scratch.root(), scratch.makeDirectory("export")};
    std::atomic<bool> progressed = false;
    std::jthread controller{[&] {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
        while (!editor.mainEntered.load() && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
        }
        const auto before = editor.timingMetrics();
        if (editor.mainEntered.load() && before.renderingActive) {
            while (std::chrono::steady_clock::now() < deadline) {
                const auto after = editor.timingMetrics();
                if (after.renderingActive && after.simulation.tick >= before.simulation.tick + 3 && after.render.frame >= before.render.frame + 3) {
                    progressed = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds{1});
            }
        }
        editor.releaseMain = true;
        if (!progressed.load()) {
            editor.requestStop();
        }
    }};
    const int result = editor.run();
    controller.join();
    CHECK(result == 0);
    CHECK(progressed.load());
    CHECK(editor.inputConsumed.load());
    CHECK_FALSE(editor.mainTimedOut.load());
    CHECK_FALSE(editor.timingMetrics().renderingActive);
}
