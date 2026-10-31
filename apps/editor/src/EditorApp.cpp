#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/platform/Platform.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>

#include <EditorApp.hpp>
#include <ui/EditorTheme.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr std::string_view EDITOR_TITLE = "TechEngine Editor";
    static constexpr std::string_view MANIFEST_PATH = "project://project.toml";
    static constexpr std::string_view STARTUP_SCENE_PATH = "project://assets/common/Startup.tescene";
    static constexpr std::string_view TRIANGLE_MESH_PATH = "project://assets/client/Triangle.temesh";
    static constexpr std::string_view LAYOUT_FOLDER = "project://editorSettings";
    static constexpr std::string_view LAYOUT_PATH = "project://editorSettings/layout.ini";

    EditorApp::EditorApp(std::filesystem::path projectRoot, std::filesystem::path exportFolder)
        : ClientApp(Role::Client, std::string(EDITOR_TITLE)), m_projectRoot(std::move(projectRoot)), m_exportFolder(std::move(exportFolder)), m_source(m_files, m_assets), m_commands(*this, m_session, m_registry, m_files), m_panels(m_commands) {
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
        // The first NewFrame builds the default layout when no dock node exists, so a later load loses to it.
        loadLayout();
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
                AssetRecord record;
                const bool indexed = m_assets.find(startupScene, record);
                if (indexed) {
                    m_session.setSceneFile(SceneFile{startupScene, record.path});
                } else {
                    TE_LOGGER_WARN("The startup scene {0:#018x} loaded but is not in the asset index, so it cannot be saved", startupScene.value());
                }
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
        // Before the loop's next shouldClose() check, which would stop the app without asking.
        if (shouldClose()) {
            handleCloseRequest();
        }
        buildImGuiFrame();
    }

    void EditorApp::publishEdits(const Scene& scene) {
        EditorSnapshot snapshot;
        m_session.buildSnapshot(scene, m_registry, snapshot);
        logSelectedTransform(scene, snapshot);
        m_snapshotSlot.publish(std::move(snapshot));
        wakeMainThread();
    }

    void EditorApp::logSelectedTransform(const Scene& scene, const EditorSnapshot& snapshot) {
        const Entity selected = snapshot.selected;
        if (snapshot.editRevision == m_loggedEditRevision || !scene.contains(selected)) {
            return;
        }
        m_loggedEditRevision = snapshot.editRevision;

        // Read from the live Scene after the batch, so the world values show the scene's own propagation.
        const Transform& transform = scene.getComponent<Transform>(selected);
        const TransformValues& local = transform.getLocal();
        const Vec3 euler = Transform::toEulerDegrees(local.rotation);
        const Vec3 world = transform.getWorld().position;
        TE_LOGGER_DEBUG(
            "Scene: entity {0} local position ({1:.3f}, {2:.3f}, {3:.3f}) rotation ({4:.3f}, {5:.3f}, {6:.3f}) scale ({7:.3f}, {8:.3f}, {9:.3f}), world position ({10:.3f}, {11:.3f}, {12:.3f})",
            selected,
            local.position.x,
            local.position.y,
            local.position.z,
            euler.x,
            euler.y,
            euler.z,
            local.scale.x,
            local.scale.y,
            local.scale.z,
            world.x,
            world.y,
            world.z);
    }

    bool EditorApp::takeSnapshot(EditorSnapshot& out) {
        return m_snapshotSlot.take(out);
    }

    EditSession& EditorApp::getSession() {
        return m_session;
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
        applyEditorTheme();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.IniFilename = nullptr;
    }

    void EditorApp::initImGuiPlatform() {
        const bool initialized = ImGui_ImplGlfw_InitForOpenGL(nativeWindow(), true);
        TE_CHECK(initialized, "Failed to init ImGui_ImplGlfw");
    }

    void EditorApp::loadLayout() {
        std::vector<std::byte> bytes;
        const std::error_code readError = m_files.read(LAYOUT_PATH, bytes);
        if (readError == FileError::NotFound) {
            return;
        }
        if (readError) {
            TE_LOGGER_WARN("Failed to read {0}, so the editor opens on the default layout: {1}", LAYOUT_PATH, readError.message());
            return;
        }
        // LoadIniSettingsFromMemory reads a size of 0 as "up to the terminating zero", and an empty file has no data pointer.
        if (!bytes.empty()) {
            ImGui::LoadIniSettingsFromMemory(reinterpret_cast<const char*>(bytes.data()), bytes.size());
        }
    }

    void EditorApp::saveLayout() {
        ImGui::GetIO().WantSaveIniSettings = false;
        std::size_t size = 0;
        const char* text = ImGui::SaveIniSettingsToMemory(&size);

        const std::error_code folderError = m_files.createDirectory(LAYOUT_FOLDER);
        if (folderError && folderError != FileError::AlreadyExists) {
            TE_LOGGER_WARN("Failed to create {0}, so the layout is not saved: {1}", LAYOUT_FOLDER, folderError.message());
            return;
        }
        const std::span bytes{reinterpret_cast<const std::byte*>(text), size};
        const std::error_code writeError = m_files.write(LAYOUT_PATH, bytes);
        if (writeError) {
            TE_LOGGER_WARN("Failed to save the layout to {0}: {1}", LAYOUT_PATH, writeError.message());
        }
    }

    void EditorApp::handleCloseRequest() {
        if (m_snapshot.hasUnsavedEdits()) {
            cancelClose();
            m_closePrompt.open();
        }
    }

    void EditorApp::buildImGuiFrame() {
        if (stopRequested()) {
            return;
        }

        takeSnapshot(m_snapshot);
        updateTitle();
        m_imguiBridge.beginFrame();
        const ImGuiContext& context = *ImGui::GetCurrentContext();
        const bool hadInput = !context.InputEventsQueue.empty();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        m_panels.draw(m_snapshot);
        drawClosePrompt();
        ImGui::Render();
        m_imguiBridge.publish(*ImGui::GetDrawData());
        if (ImGui::GetIO().WantSaveIniSettings) {
            saveLayout();
        }

        const bool inputLeft = !context.InputEventsQueue.empty();
        const bool holdingItem = ImGui::IsAnyItemActive() && ImGui::IsMouseDown(ImGuiMouseButton_Left);
        if (hadInput || inputLeft || holdingItem) {
            requestMainFrame();
        }
    }

    void EditorApp::updateTitle() {
        std::string title(EDITOR_TITLE);
        if (m_snapshot.hasUnsavedEdits()) {
            title += "*";
        }
        setTitle(std::move(title));
    }

    void EditorApp::drawClosePrompt() {
        const CloseChoice choice = m_closePrompt.draw();
        switch (choice) {
            case CloseChoice::Save:
                m_commands.saveAndClose();
                break;
            case CloseChoice::Discard:
                requestStop();
                break;
            case CloseChoice::Cancel:
            case CloseChoice::None:
                break;
        }
    }

    void EditorApp::stopImGui() {
        const bool hasContext = ImGui::GetCurrentContext() != nullptr;
        if (hasContext && ImGui::GetIO().BackendPlatformUserData != nullptr) {
            ImGui_ImplGlfw_Shutdown();
        }
        stopClient();
        if (hasContext) {
            // ImGui raises WantSaveIniSettings only seconds after a change, so a panel moved just before closing is still unsaved.
            saveLayout();
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
            m_session.setSceneFile(SceneFile{id, std::string(STARTUP_SCENE_PATH)});
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
