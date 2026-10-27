#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/core/assets/PackSource.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/BootRecord.hpp>
#include <TechEngine/core/resources/PackReader.hpp>
#include <TechEngine/core/scene/SceneAsset.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/core/serialization/ReadError.hpp>

#include <RuntimeApp.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace TechEngine {
    static constexpr std::string_view ASSETS_DIRECTORY = "assets://";
    static constexpr std::string_view COMMON_PACK_PATH = "assets://common.tepack";

    RuntimeApp::RuntimeApp(std::filesystem::path folder) : ClientApp(Role::Client, "TechEngine Runtime Demo"), m_folder(std::move(folder)), m_source(m_files) {
    }

    void RuntimeApp::init() {
        mountAssets();
        startClient();
    }

    void RuntimeApp::mountAssets() {
        m_mounts.mount("assets", m_folder / "assets");
    }

    void RuntimeApp::configureSimulation() {
        ClientApp::configureSimulation();
        m_registry.registerComponent<MeshRenderer>(MeshRenderer::tag);
        m_resources.registerLoader<SceneBytes>(SCENE_KIND, SCENE_TYPE_TAG, &SceneBytes::loadPayload);
        m_resources.registerLoader<Mesh>(MESH_KIND, MESH_TYPE_TAG, &Mesh::loadPayload);
    }

    std::error_code RuntimeApp::simulationStartupLoad(Scene& scene) {
        PackReader common(m_files);
        const std::error_code commonError = common.open(COMMON_PACK_PATH);
        if (commonError) {
            TE_LOGGER_ERROR("Failed to open {0}: {1}", COMMON_PACK_PATH, commonError.message());
            return commonError;
        }

        PackEntry bootEntry;
        if (!common.bootRecord(bootEntry)) {
            TE_LOGGER_ERROR("{0} holds no boot record", COMMON_PACK_PATH);
            return ReadError::Malformed;
        }
        std::vector<std::byte> bootBytes;
        const std::error_code bootReadError = common.read(bootEntry, bootBytes);
        if (bootReadError) {
            return bootReadError;
        }
        BootRecord record;
        const std::error_code bootParseError = record.read(bootBytes);
        if (bootParseError) {
            TE_LOGGER_ERROR("The boot record in {0} does not parse: {1}", COMMON_PACK_PATH, bootParseError.message());
            return bootParseError;
        }
        if (!record.startupScene.valid()) {
            TE_LOGGER_ERROR("The boot record in {0} names no startup scene", COMMON_PACK_PATH);
            return ReadError::Malformed;
        }

        m_resources.setSource(m_source);
        const std::error_code sourceError = m_source.open(record, Role::Client, ASSETS_DIRECTORY);
        if (sourceError) {
            TE_LOGGER_ERROR("Failed to open the client's packs under {0}: {1}", ASSETS_DIRECTORY, sourceError.message());
            return sourceError;
        }
        if (m_source.find(record.startupScene) != AssetAvailability::Present) {
            TE_LOGGER_ERROR("The client's packs do not hold the startup scene {0:#018x}", record.startupScene.value());
            return ReadError::Malformed;
        }

        const std::error_code loadError = SceneAsset{record.startupScene}.load(m_resources, scene, m_sceneHandle);
        if (loadError) {
            TE_LOGGER_ERROR("The startup scene {0:#018x} does not load: {1}", record.startupScene.value(), loadError.message());
            return loadError;
        }

        const std::vector<Entity> roots = scene.getRoots();
        TE_LOGGER_INFO("Loaded the startup scene {0:#018x} with {1} root entities", record.startupScene.value(), roots.size());
        for (const Entity root: roots) {
            logSceneTree(scene, root, 1);
        }
        return {};
    }

    void RuntimeApp::logSceneTree(const Scene& scene, const Entity entity, const std::size_t depth) const {
        const Vec3& position = scene.getComponent<Transform>(entity).getLocal().position;
        const Name& nameComponent = scene.getComponent<Name>(entity);
        std::string name = nameComponent.value;
        TE_LOGGER_INFO("{0}Entity {1} {2}:{3} at ({4}, {5}, {6})", std::string(depth * 2, ' '), name, entity.index, entity.generation, position.x, position.y, position.z);
        if (scene.hasComponent<MeshRenderer>(entity)) {
            const AssetRef<Mesh>& reference = scene.getComponent<MeshRenderer>(entity).mesh;
            const Mesh* mesh = reference.handle.valid() ? m_resources.resolve(reference.handle) : nullptr;
            if (mesh != nullptr) {
                TE_LOGGER_INFO("{0}Mesh {1:#018x} with {2} positions and {3} indices", std::string(depth * 2 + 2, ' '), reference.id.value(), mesh->positions.size(), mesh->indices.size());
            } else {
                TE_LOGGER_INFO("{0}Mesh {1:#018x}, not loaded", std::string(depth * 2 + 2, ' '), reference.id.value());
            }
        }
        for (const Entity child: scene.getChildren(entity)) {
            logSceneTree(scene, child, depth + 1);
        }
    }
}
