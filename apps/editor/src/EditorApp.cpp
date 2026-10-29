#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/platform/Platform.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>

#include <EditorApp.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr std::string_view MANIFEST_PATH = "project://project.toml";
    static constexpr std::string_view STARTUP_SCENE_PATH = "project://assets/common/Startup.tescene";
    static constexpr std::string_view TRIANGLE_MESH_PATH = "project://assets/client/Triangle.temesh";

    EditorApp::EditorApp(std::filesystem::path projectRoot, std::filesystem::path exportFolder)
        : ClientApp(Role::Client, "TechEngine Editor"), m_projectRoot(std::move(projectRoot)), m_exportFolder(std::move(exportFolder)), m_source(m_files, m_assets), m_commands(*this, m_session), m_panels(m_commands) {
    }

    EditorApp::~EditorApp() {
        stopImGui();
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

        // The hook's start runs inside startClient() and needs the context; the platform backend needs the open window.
        createImGuiContext();
        startClient(&m_imguiBridge);
        initImGuiPlatform();
        // Nothing consumes the input buffer while the simulation is Stopped, so an attached one fills until it overflows.
        detachInput();
    }

    void EditorApp::configureSimulation() {
        ClientApp::configureSimulation();
        m_registry.registerComponent<MeshRenderer>(MeshRenderer::tag);
        m_resources.registerLoader<SceneBytes>(SCENE_KIND, SCENE_TYPE_TAG, &SceneBytes::loadPayload);
        m_resources.registerLoader<Mesh>(MESH_KIND, MESH_TYPE_TAG, &Mesh::loadPayload);
    }

    bool EditorApp::simulationTicks() const {
        return false;
    }

    std::error_code EditorApp::simulationStartupLoad(Scene& scene) {
        m_resources.setSource(m_source);
        const ResourceId startupScene = m_project.getExportStartupScene(Role::Client);
        if (startupScene.valid()) {
            const std::error_code loadError = SceneAsset{startupScene}.load(m_resources, scene, m_sceneHandle);
            if (loadError) {
                TE_LOGGER_ERROR("The startup scene {0:#018x} does not load, so the editor opens on an empty scene: {1}", startupScene.value(), loadError.message());
            } else {
                TE_LOGGER_INFO("Opened the startup scene {0:#018x} with {1} root entities", startupScene.value(), scene.getRoots().size());
            }
        } else {
            createStartupScene(scene);
        }

        // TODO: Remove the export step from the startup load and make it a separate command in the editor UI.
        const std::error_code exportError = m_project.exportTo(Role::Client, m_assets, m_files, m_registry, m_exportFolder);
        if (exportError) {
            TE_LOGGER_ERROR("Failed to export the project to {0}: {1}", m_exportFolder.string(), exportError.message());
        }

        // Stopped, the editor gets no batch until the first edit, so without this the panels start empty.
        publishEdits(scene);
        return {};
    }

    void EditorApp::mainThreadUpdate() {
        ClientApp::mainThreadUpdate();
        buildImGuiFrame();
    }

    void EditorApp::publishEdits(const Scene& scene) {
        EditorSnapshot snapshot;
        m_session.buildSnapshot(scene, snapshot);
        m_snapshotSlot.publish(std::move(snapshot));
        wakeMainThread();
    }

    bool EditorApp::takeSnapshot(EditorSnapshot& out) {
        return m_snapshotSlot.take(out);
    }

    void EditorApp::shutdown() {
        stopImGui();
        ClientApp::shutdown();
    }

    const AssetIndex& EditorApp::getAssetIndex() const {
        return m_assets;
    }

    void EditorApp::createImGuiContext() {
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename = nullptr;
    }

    void EditorApp::initImGuiPlatform() {
        const bool initialized = ImGui_ImplGlfw_InitForOpenGL(nativeWindow(), true);
        TE_CHECK(initialized, "Failed to init ImGui_ImplGlfw");
    }

    void EditorApp::buildImGuiFrame() {
        if (stopRequested()) {
            return;
        }

        takeSnapshot(m_snapshot);
        m_imguiBridge.beginFrame();
        const ImGuiContext& context = *ImGui::GetCurrentContext();
        const bool hadInput = !context.InputEventsQueue.empty();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        m_panels.draw(m_snapshot);
        ImGui::Render();
        m_imguiBridge.publish(*ImGui::GetDrawData());

        // NewFrame trickles queued input, so a press and release in one wake take two frames.
        const bool inputLeft = !context.InputEventsQueue.empty();
        if (hadInput || inputLeft || ImGui::IsAnyItemActive()) {
            wakeMainThread();
        }
    }

    void EditorApp::stopImGui() {
        const bool hasContext = ImGui::GetCurrentContext() != nullptr;
        if (hasContext && ImGui::GetIO().BackendPlatformUserData != nullptr) {
            ImGui_ImplGlfw_Shutdown();
        }
        stopClient();
        if (hasContext) {
            ImGui::DestroyContext();
        }
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

    ResourceId EditorApp::writeStartupScene(Scene& scene) {
        // Checked before the mesh is created, so a refused scene leaves no mesh behind.
        requireNewAssetPath(STARTUP_SCENE_PATH);
        const ResourceId meshId = findOrCreateTriangleMesh();

        const Entity parent = scene.createEntity();
        const Entity firstChild = scene.createEntity();
        const Entity secondChild = scene.createEntity();
        const bool firstParented = scene.setParent(firstChild, parent, 0);
        TE_CHECK(firstParented, "Failed to parent the startup scene's first child");
        const bool secondParented = scene.setParent(secondChild, parent, 1);
        TE_CHECK(secondParented, "Failed to parent the startup scene's second child");
        scene.addComponent<MeshRenderer>(parent, MeshRenderer{AssetRef<Mesh>{meshId, {}}});

        return createAsset(STARTUP_SCENE_PATH, SCENE_TYPE_TAG, Side::Common, [&scene](const ResourceId id, std::vector<std::byte>& bytes) {
            SceneAsset{id}.write(scene, bytes);
        });
    }

    ResourceId EditorApp::findOrCreateTriangleMesh() {
        const std::vector<AssetRecord>& records = m_assets.getRecords();
        const auto existing = std::ranges::find_if(records, [](const AssetRecord& record) {
            return record.path == TRIANGLE_MESH_PATH && record.typeTag == MESH_TYPE_TAG;
        });
        if (existing != records.end()) {
            return existing->id;
        }

        return createAsset(TRIANGLE_MESH_PATH, MESH_TYPE_TAG, Side::Client, [](const ResourceId id, std::vector<std::byte>& bytes) {
            const Mesh mesh{id, {Vec3(-0.5f, -0.5f, 0.0f), Vec3(0.5f, -0.5f, 0.0f), Vec3(0.0f, 0.5f, 0.0f)}, {0, 1, 2}};
            mesh.write(bytes);
        });
    }

    ResourceId EditorApp::createAsset(const std::string_view path, const StringId typeTag, const Side side, const std::function<void(ResourceId, std::vector<std::byte>&)>& write) {
        requireNewAssetPath(path);

        const ResourceId id = m_assets.generateId();
        std::vector<std::byte> bytes;
        write(id, bytes);
        const std::error_code writeError = m_files.write(path, bytes);
        TE_CHECK(!writeError, "Failed to write {0}: {1}", path, writeError.message());

        AssetRecord record;
        record.id = id;
        record.typeTag = typeTag;
        record.side = side;
        record.path = std::string(path);
        m_assets.add(std::move(record));
        return id;
    }

    void EditorApp::requireNewAssetPath(const std::string_view path) const {
        FileStatus status;
        const std::error_code statusError = m_files.status(path, status);
        TE_CHECK(statusError == FileError::NotFound, "{0} exists but is not an indexed asset, so it is not overwritten", path);
    }
}
