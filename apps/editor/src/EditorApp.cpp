#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshReference.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/platform/Platform.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>
#include <TechEngine/platform/files/FileError.hpp>

#include <EditorApp.hpp>

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

    EditorApp::EditorApp(std::filesystem::path projectRoot, std::filesystem::path exportFolder) : ClientApp(Role::Client, "TechEngine Editor"), m_projectRoot(std::move(projectRoot)), m_exportFolder(std::move(exportFolder)) {
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

        startClient();
    }

    void EditorApp::configureSimulation() {
        ClientApp::configureSimulation();
        m_registry.registerComponent<MeshReference>(MeshReference::tag);
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

    const AssetIndex& EditorApp::getAssetIndex() const {
        return m_assets;
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
        scene.addComponent<MeshReference>(parent, MeshReference{AssetRef<Mesh>{meshId, {}}});

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
