#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/diagnostics/Profile.hpp>
#include <TechEngine/client/render/RenderSnapshot.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/platform/Platform.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>

#include <EditorApp.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr std::string_view MANIFEST_PATH = "project://project.toml";
    static constexpr std::string_view STARTUP_SCENE_PATH = "project://assets/common/Startup.tescene";

    EditorApp::EditorApp(std::filesystem::path projectRoot, std::filesystem::path exportFolder) : App(editorRole()), m_projectRoot(std::move(projectRoot)), m_exportFolder(std::move(exportFolder)) {
    }

    void EditorApp::init() {
        m_mounts.mount("project", m_projectRoot);
        m_mounts.mount("engine", executablePath().parent_path() / "engine");
        const std::error_code loadError = m_project.load(m_files, MANIFEST_PATH);
        TE_CHECK(!loadError, "Failed to load project.toml under {0}: {1}", m_projectRoot.string(), loadError.message());

        const auto& root = m_project.root();
        m_mounts.mount("assets", root / "assets" / "common", 0);
        m_mounts.mount("assets", root / "assets" / "client", 100);
        const std::error_code indexError = m_assets.build(m_files, "project://assets");
        TE_CHECK(!indexError, "Failed to index the assets under {0}: {1}", root.string(), indexError.message());
        TE_LOGGER_INFO("Indexed {0} assets with {1} duplicates", m_assets.getRecords().size(), m_assets.getDuplicates().size());
        TE_LOGGER_INFO("Opened project '{0}' at {1}", m_project.name(), root.string());

        const std::error_code startError = m_client.start(m_engine, m_input, 1280, 720, "TechEngine Editor", [this] {
            requestStop();
        });
        if (startError) {
            throw std::runtime_error{"Failed to start the client session: " + startError.message()};
        }
    }

    std::error_code EditorApp::simulationStartupLoad(Scene& scene) {
        if (m_project.getExportStartupScene(Role::Client).valid()) {
            // TODO: read the startup scene into the scene with SceneAsset::read, so the editor opens on
            // it. Until then a later launch leaves the scene empty.
        } else {
            createStartupScene(scene);
        }

        // TODO: Remove the export step from the startup load and make it a separate command in the editor UI.
        const std::error_code exportError = m_project.exportTo(Role::Client, m_assets, m_files, m_exportFolder);
        if (exportError) {
            TE_LOGGER_ERROR("Failed to export the project to {0}: {1}", m_exportFolder.string(), exportError.message());
        }
        return {};
    }

    void EditorApp::publishSnapshot(const SimulationContext& simulation) {
        m_client.publish(RenderSnapshot{
            .clearColor = {0.1F, 0.1F, 0.1F, 1.0F},
            .drawTriangle = true,
            .tick = simulation.tick,
            .tickTime = simulation.tickTime,
            .fixedDeltaTime = simulation.fixedDeltaTime,
            .timeline = simulation.timeline,
            .input = simulation.input.held,
        });
    }

    void EditorApp::mainThreadUpdate() {
        if (stopRequested()) {
            return;
        }
        {
            TE_PROFILER_SCOPE("Main.WaitEvents");
            m_client.waitEvents();
        }
        const auto metrics = timingMetrics();
        const auto fps = static_cast<std::uint64_t>(metrics.renderingActive ? metrics.render.framesPerSecond : 0.0);
        const auto tps = static_cast<std::uint64_t>(metrics.simulation.ticksPerSecond);
        std::string title = "TechEngine Editor | FPS: " + std::to_string(fps) + " | TPS: " + std::to_string(tps);
        if (title != m_appliedTitle) {
            m_client.setTitle(title);
            m_appliedTitle = std::move(title);
        }
        if (m_client.failed()) {
            requestStop();
        }
    }

    void EditorApp::wakeMainThread() {
        m_client.wakeMain();
    }

    bool EditorApp::renderTiming(RenderTiming& out) const {
        return m_client.renderTiming(out);
    }

    void EditorApp::shutdown() {
        m_client.stop();
        if (m_client.failed()) {
            throw std::runtime_error{"Render thread failed"};
        }
    }

    bool EditorApp::shouldClose() const {
        return m_client.shouldClose();
    }

    const AssetIndex& EditorApp::getAssetIndex() const {
        return m_assets;
    }

    Role EditorApp::editorRole() {
        return Role::Client;
    }

    void EditorApp::createStartupScene(Scene& scene) {
        const std::vector<AssetRecord>& records = m_assets.getRecords();
        const auto existing = std::ranges::find(records, STARTUP_SCENE_PATH, &AssetRecord::path);
        ResourceId id;
        if (existing != records.end()) {
            TE_LOGGER_WARN("{0} names no startup scene, so the existing {1} becomes it", MANIFEST_PATH, STARTUP_SCENE_PATH);
            id = existing->id;
        } else {
            id = writeStartupScene(scene);
        }

        m_project.setExportStartupScene(Role::Client, id);
        const std::error_code saveError = m_project.save(m_files, MANIFEST_PATH);
        TE_CHECK(!saveError, "Failed to save {0}: {1}", MANIFEST_PATH, saveError.message());
    }

    // TODO: move the id, file write and index record into a shared create function when the next
    // asset kind gets its writer, so the content browser can create scenes through it too.
    ResourceId EditorApp::writeStartupScene(Scene& scene) {
        FileStatus status;
        const std::error_code statusError = m_files.status(STARTUP_SCENE_PATH, status);
        TE_CHECK(statusError == FileError::NotFound, "{0} exists but is not an indexed asset, so it is not overwritten", STARTUP_SCENE_PATH);

        const Entity parent = scene.createEntity();
        const Entity firstChild = scene.createEntity();
        const Entity secondChild = scene.createEntity();
        const bool firstParented = scene.setParent(firstChild, parent, 0);
        TE_CHECK(firstParented, "Failed to parent the startup scene's first child");
        const bool secondParented = scene.setParent(secondChild, parent, 1);
        TE_CHECK(secondParented, "Failed to parent the startup scene's second child");

        const ResourceId id = m_assets.generateId();
        std::vector<std::byte> bytes;
        SceneAsset{id}.write(scene, bytes);
        const std::error_code writeError = m_files.write(STARTUP_SCENE_PATH, bytes);
        TE_CHECK(!writeError, "Failed to write the startup scene to {0}: {1}", STARTUP_SCENE_PATH, writeError.message());

        AssetRecord record;
        record.id = id;
        record.typeTag = SCENE_TYPE_TAG;
        record.side = Side::Common;
        record.path = std::string(STARTUP_SCENE_PATH);
        m_assets.add(std::move(record));
        return id;
    }
}
