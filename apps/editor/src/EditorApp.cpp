#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/assets/ImportBlock.hpp>
#include <TechEngine/core/material/Material.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/texture/Texture.hpp>
#include <TechEngine/platform/Platform.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>

#include <EditorApp.hpp>
#include <assets/ModelImporter.hpp>
#include <edit/ExportCommand.hpp>
#include <edit/ImportImageCommand.hpp>
#include <edit/ImportModelCommand.hpp>
#include <edit/MaterialSlotRebuild.hpp>
#include <edit/ReimportCommand.hpp>
#include <ui/EditorTheme.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
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
    static constexpr std::string_view STARTUP_IMPORT_PATH = "project://assets/client/Sponza.gltf";
    static constexpr Side STARTUP_IMPORT_SIDE = Side::Client;
    static constexpr ImportSettings STARTUP_IMPORT_SETTINGS{};
    static constexpr ImportMode STARTUP_IMPORT_MODE = ImportMode::Model;
    static constexpr std::string_view LAYOUT_FOLDER = "project://editorSettings";
    static constexpr std::string_view LAYOUT_PATH = "project://editorSettings/layout.ini";

    EditorApp::EditorApp(std::filesystem::path projectRoot, std::filesystem::path exportFolder, std::filesystem::path runtimePath)
        : ClientApp(Role::Client, std::string(EDITOR_TITLE)), m_projectRoot(std::move(projectRoot)), m_exportFolder(std::move(exportFolder)), m_runtimePath(std::move(runtimePath)), m_source(m_files, m_assets),
          m_commands(*this, m_session, m_registry, m_files, m_resources, m_sceneHandle, m_assets), m_panels(
                                                                                                       m_commands,
                                                                                                       m_consoleHistory,
                                                                                                       [this] {
                                                                                                           exportProject();
                                                                                                       },
                                                                                                       [this] {
                                                                                                           runProject();
                                                                                                       },
                                                                                                       [this](std::string path) {
                                                                                                           importImage(std::move(path));
                                                                                                       },
                                                                                                       [this](std::string path, const ImportMode mode) {
                                                                                                           reimport(std::move(path), mode);
                                                                                                       }),
          m_console([this] {
              wakeMainThread();
          }) {
    }

    EditorApp::~EditorApp() {
        // Keeps a running import job off the members destroyed below. It comes too late for the job's log lines: the
        // console wakes main through a virtual call, which races this destructor, so shutdown() must have waited first.
        waitForImports();
        stopImGui();
    }

    void EditorApp::init() {
        const std::error_code consoleError = m_console.start();
        if (consoleError) {
            TE_LOGGER_WARN("The console shows no log, because its sink did not register: {0}", consoleError.message());
        }
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
        m_exportedRuntimePath = m_exportFolder / m_project.name() / m_runtimePath.filename();

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
        m_resources.registerLoader<Texture>(TEXTURE_KIND, TEXTURE_TYPE_TAG, &Texture::loadPayload);
        m_resources.registerLoader<Material>(MATERIAL_KIND, MATERIAL_TYPE_TAG, &Material::loadPayload);
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
                rebuildMaterialSlots(scene, m_resources, m_sceneHandle);
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
        importStartupModel(scene);

        // Stopped, the editor gets no batch until the first edit, so without this the panels start empty.
        publishEdits(scene);
        return {};
    }

    void EditorApp::mainThreadUpdate() {
        ClientApp::mainThreadUpdate();
        pollRuntime();
        // Before the loop's next shouldClose() check, which would stop the app without asking.
        if (shouldClose()) {
            handleCloseRequest();
        }
        buildImGuiFrame();
    }

    void EditorApp::publishEdits(const Scene& scene) {
        EditorSnapshot snapshot;
        m_session.buildSnapshot(scene, m_registry, snapshot);
        snapshot.assets = m_assets.getRecords();
        logSelectedTransform(scene, snapshot);
        m_snapshotSlot.publish(std::move(snapshot));
        wakeMainThread();
    }

    void EditorApp::logSelectedTransform(const Scene& scene, const EditorSnapshot& snapshot) {
        const Entity selected = snapshot.active;
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

    std::unique_ptr<Command> EditorApp::makeExportCommand(std::function<void(bool exported)> onFinished) {
        return std::make_unique<ExportCommand>(m_session, m_project, m_assets, m_files, m_registry, m_runtimePath, m_exportFolder, std::move(onFinished));
    }

    std::unique_ptr<Command> EditorApp::makeRunCommand() {
        return makeExportCommand([this](const bool exported) {
            m_runExport.store(exported ? RunExport::Succeeded : RunExport::Failed);
            wakeMainThread();
        });
    }

    std::unique_ptr<Command> EditorApp::makeImportModelCommand(ModelImportRequest request) {
        return std::make_unique<ImportModelCommand>(*this, m_jobs, m_files, m_assets, m_importBatches, std::move(request));
    }

    void EditorApp::waitForImports() {
        for (const BatchId batch: m_importBatches) {
            m_jobs.wait(batch);
        }
        m_importBatches.clear();
    }

    bool EditorApp::canRun() const {
        return !m_runPending && !m_runtime.isRunning();
    }

    void EditorApp::runProject() {
        // Set before the submit, because the simulation thread can finish the export before submitCommand returns.
        m_runPending = true;
        const std::error_code submitError = submitCommand(makeRunCommand());
        if (submitError) {
            m_runPending = false;
            TE_LOGGER_WARN("The simulation refused the export Run starts with: {0}", submitError.message());
        }
    }

    void EditorApp::shutdown() {
        // Before the console stops, so a job's last log lines still reach it.
        waitForImports();
        // Before stopImGui(): the client holds its wake lock while GLFW may log an error, and a
        // console write from another thread takes that lock to wake main.
        const std::error_code consoleError = m_console.stop();
        if (consoleError) {
            TE_LOGGER_WARN("Failed to remove the console's log sink: {0}", consoleError.message());
        }
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

        std::vector<StoredLogRecord> consoleRecords;
        const std::size_t droppedRecords = m_console.drain(consoleRecords);
        m_consoleHistory.add(std::move(consoleRecords), droppedRecords);
        takeSnapshot(m_snapshot);
        updateTitle();
        m_imguiBridge.beginFrame();
        const ImGuiContext& context = *ImGui::GetCurrentContext();
        const bool hadInput = !context.InputEventsQueue.empty();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        m_panels.draw(m_snapshot, canRun());
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

    void EditorApp::exportProject() {
        const std::error_code submitError = submitCommand(makeExportCommand({}));
        if (submitError) {
            TE_LOGGER_WARN("The simulation refused the export: {0}", submitError.message());
        }
    }

    void EditorApp::importImage(std::string path) {
        ImageImportRequest request;
        request.sourcePath = std::move(path);
        const std::error_code submitError = submitCommand(std::make_unique<ImportImageCommand>(*this, m_jobs, m_files, m_assets, m_importBatches, std::move(request)));
        if (submitError) {
            TE_LOGGER_WARN("The simulation refused the image import: {0}", submitError.message());
        }
    }

    void EditorApp::reimport(std::string path, const ImportMode mode) {
        ReimportRequest request;
        request.sourcePath = std::move(path);
        request.mode = mode;
        const std::error_code submitError = submitCommand(std::make_unique<ReimportCommand>(*this, m_jobs, m_files, m_assets, m_session, m_resources, m_sceneHandle, m_importBatches, std::move(request)));
        if (submitError) {
            TE_LOGGER_WARN("The simulation refused the re-import: {0}", submitError.message());
        }
    }

    void EditorApp::startRuntime() {
        const std::error_code startError = m_runtime.start(m_exportedRuntimePath, m_exportedRuntimePath.parent_path());
        if (startError) {
            TE_LOGGER_ERROR("Failed to start the runtime at {0}: {1}", m_exportedRuntimePath.string(), startError.message());
            return;
        }
        TE_LOGGER_INFO("Started the runtime at {0}", m_exportedRuntimePath.string());
    }

    void EditorApp::pollRuntime() {
        const RunExport runExport = m_runExport.exchange(RunExport::None);
        if (runExport != RunExport::None) {
            m_runPending = false;
            if (runExport == RunExport::Succeeded) {
                startRuntime();
            } else {
                TE_LOGGER_ERROR("Run did not start the runtime, because its export failed");
            }
        }

        // An idle editor waits for its next event, so an exit while nothing happens is logged on the next one.
        int exitCode = 0;
        const bool exited = m_runtime.pollExit(exitCode);
        if (exited) {
            TE_LOGGER_INFO("The runtime exited with code {0}", exitCode);
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
            scene.clear();
            const std::error_code loadError = SceneAsset{id}.load(m_resources, scene, m_sceneHandle);
            if (loadError) {
                TE_LOGGER_ERROR("The startup scene {0:#018x} was written but does not load back, so the editor opens on an empty scene: {1}", id.value(), loadError.message());
            } else {
                rebuildMaterialSlots(scene, m_resources, m_sceneHandle);
                m_session.setSceneFile(SceneFile{.id = id, .path = std::string(STARTUP_SCENE_PATH)});
            }
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
            Mesh mesh;
            mesh.id = id;
            mesh.positions = {Vec3(-0.5f, -0.5f, 0.0f), Vec3(0.5f, -0.5f, 0.0f), Vec3(0.0f, 0.5f, 0.0f)};
            mesh.indices = {0, 1, 2};
            mesh.slots = {MeshSlot{"Slot 0", 0, 3, ResourceId{}}};
            mesh.write(bytes);
        });
    }

    void EditorApp::importStartupModel(Scene& scene) {
        // TODO(E2): the asset browser's import replaces this hardcoded file.
        FileStatus source;
        const std::error_code sourceError = m_files.status(STARTUP_IMPORT_PATH, source);
        if (sourceError) {
            return;
        }
        FileStatus folder;
        const std::error_code folderError = m_files.status(ModelImporter::importFolder(STARTUP_IMPORT_PATH), folder);
        if (folderError != FileError::NotFound) {
            return;
        }

        ModelImportRequest request;
        request.sourcePath = std::string(STARTUP_IMPORT_PATH);
        request.side = STARTUP_IMPORT_SIDE;
        request.settings = STARTUP_IMPORT_SETTINGS;
        request.mode = STARTUP_IMPORT_MODE;
        makeImportModelCommand(std::move(request))->apply(scene);
    }

    ResourceId EditorApp::createAsset(const std::string_view path, const StringId typeTag, const Side side, const std::function<void(ResourceId, std::vector<std::byte>&)>& write) {
        requireNewAssetPath(path);

        const ResourceId id = m_assets.generateId();
        std::vector<std::byte> bytes;
        write(id, bytes);
        const std::error_code writeError = m_files.write(path, bytes);
        TE_CHECK(!writeError, "Failed to write {0}: {1}", path, writeError.message());

        m_assets.add(AssetRecord{id, typeTag, side, std::string(path)});
        return id;
    }

    void EditorApp::requireNewAssetPath(const std::string_view path) const {
        FileStatus status;
        const std::error_code statusError = m_files.status(path, status);
        TE_CHECK(statusError == FileError::NotFound, "{0} exists but is not an indexed asset, so it is not overwritten", path);
    }
}
