#include <TechEngine/base/diagnostics/Assert.hpp>
#include <TechEngine/base/diagnostics/Log.hpp>
#include <TechEngine/core/mesh/Mesh.hpp>
#include <TechEngine/core/mesh/MeshRenderer.hpp>
#include <TechEngine/core/model/Model.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/resources/ResourceCache.hpp>
#include <TechEngine/core/resources/Side.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>
#include <TechEngine/platform/files/FileAccess.hpp>

#include <assets/AssetIndex.hpp>
#include <edit/EditSession.hpp>
#include <edit/MaterialSlotRebuild.hpp>
#include <edit/PlaceModelCommand.hpp>

#include <algorithm>
#include <cstddef>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace TechEngine {
    PlaceModelCommand::PlaceModelCommand(EditSession& session, const FileAccess& files, ResourceCache& resources, const ResourceHandle<SceneBytes>& sceneHandle, const AssetIndex& assets, std::string path)
        : m_session(&session), m_files(&files), m_resources(&resources), m_sceneHandle(&sceneHandle), m_assets(&assets), m_path(std::move(path)) {
    }

    void PlaceModelCommand::apply(Scene& scene) {
        if (!m_sceneHandle->valid()) {
            TE_LOGGER_WARN("The scene has no cache entry, so {0} is not placed", m_path);
            return;
        }

        Model model;
        const bool read = readModel(model);
        if (!read) {
            return;
        }

        std::unordered_map<ResourceId, ResourceSlotId> meshes;
        const bool loaded = loadMeshes(model, meshes);
        if (!loaded) {
            return;
        }

        std::vector<Entity> created;
        const bool built = buildEntities(scene, model, meshes, created);
        if (!built) {
            return;
        }

        rebuildMaterialSlots(scene, *m_resources, *m_sceneHandle, created);
        TE_LOGGER_INFO("Placed {0}: {1} entities", m_path, created.size());
        m_session->recordSceneChange();
    }

    bool PlaceModelCommand::readModel(Model& out) const {
        const std::vector<AssetRecord>& records = m_assets->getRecords();
        const auto record = std::ranges::find(records, m_path, &AssetRecord::path);
        if (record == records.end()) {
            TE_LOGGER_WARN("{0} is not in the asset index, so it is not placed", m_path);
            return false;
        }
        if (record->typeTag == MESH_TYPE_TAG) {
            wrapMesh(*record, out);
            return true;
        }
        if (record->typeTag != MODEL_TYPE_TAG) {
            TE_LOGGER_WARN("{0} is neither a model nor a mesh, so it is not placed", m_path);
            return false;
        }

        std::vector<std::byte> bytes;
        const std::error_code readError = m_files->read(m_path, bytes);
        if (readError) {
            TE_LOGGER_WARN("Failed to read {0}, so it is not placed: {1}", m_path, readError.message());
            return false;
        }
        const std::error_code parseError = out.read(bytes);
        if (parseError) {
            TE_LOGGER_WARN("{0} is not a valid model, so it is not placed: {1}", m_path, parseError.message());
            return false;
        }
        return true;
    }

    // A mesh is placed as a model of one root node, named after the file and at the origin.
    void PlaceModelCommand::wrapMesh(const AssetRecord& record, Model& out) {
        std::string_view name = record.path;
        const std::size_t slash = name.find_last_of('/');
        if (slash != std::string_view::npos) {
            name = name.substr(slash + 1);
        }
        const std::size_t dot = name.find_last_of('.');
        if (dot != std::string_view::npos && dot > 0) {
            name = name.substr(0, dot);
        }

        ModelNode node;
        node.name = std::string(name);
        node.mesh = record.id;
        out.nodes.push_back(std::move(node));
    }

    bool PlaceModelCommand::loadMeshes(const Model& model, std::unordered_map<ResourceId, ResourceSlotId>& out) const {
        std::vector<ResourceId> ids;
        for (const ModelNode& node: model.nodes) {
            if (node.mesh.valid() && std::ranges::find(ids, node.mesh) == ids.end()) {
                ids.push_back(node.mesh);
            }
        }

        for (const ResourceId id: ids) {
            AssetRecord asset;
            const bool indexed = m_assets->find(id, asset);
            if (!indexed) {
                TE_LOGGER_WARN("{0} names mesh {1:#018x}, which is not in the asset index, so it is not placed", m_path, id.value());
                return false;
            }
            if (asset.typeTag != MESH_TYPE_TAG) {
                TE_LOGGER_WARN("{0} names asset {1:#018x} as a mesh, but it is not one, so it is not placed", m_path, id.value());
                return false;
            }
            if (asset.side == Side::Server) {
                TE_LOGGER_WARN("{0} names the server mesh {1:#018x}, which the editor's client scene cannot load, so it is not placed", m_path, id.value());
                return false;
            }
        }

        for (const ResourceId id: ids) {
            ResourceSlotId slot;
            const std::error_code addError = m_resources->addDependency(*m_sceneHandle, id, slot);
            if (addError) {
                TE_LOGGER_WARN("Mesh {0:#018x} of {1} cannot be added to the scene, so it is not placed: {2}", id.value(), m_path, addError.message());
                return false;
            }
            out.emplace(id, slot);
        }

        const std::error_code loadError = m_resources->runLoadingPhase();
        for (const auto& [id, slot]: out) {
            ResourceState state = ResourceState::Pending;
            const bool found = m_resources->getState(slot, state);
            if (!found || state != ResourceState::Ready) {
                TE_LOGGER_WARN("Mesh {0:#018x} of {1} did not load, so it is not placed: {2}", id.value(), m_path, loadError ? loadError.message() : std::string("it had already failed"));
                return false;
            }
        }
        return true;
    }

    bool PlaceModelCommand::buildEntities(Scene& scene, const Model& model, const std::unordered_map<ResourceId, ResourceSlotId>& meshes, std::vector<Entity>& out) const {
        out.reserve(model.nodes.size());
        for (const ModelNode& node: model.nodes) {
            const Entity entity = scene.createEntity();
            TE_CHECK(entity.valid(), "Failed to create the entity for node {0} of {1}", node.name, m_path);
            out.push_back(entity);

            if (node.parent != MODEL_NO_PARENT) {
                const Entity parent = out[node.parent];
                const bool parented = scene.setParent(entity, parent, scene.getChildren(parent).size());
                TE_CHECK(parented, "Failed to parent node {0} of {1}", node.name, m_path);
            }

            scene.getComponent<Name>(entity).value = node.name;
            const bool localSet = scene.getComponent<Transform>(entity).setLocal(node.local);
            if (!localSet) {
                TE_LOGGER_WARN("Node {0} of {1} has a zero scale on an axis, so the model is not placed", node.name, m_path);
                removeEntities(scene, out);
                return false;
            }

            if (node.mesh.valid()) {
                const ResourceSlotId slot = meshes.at(node.mesh);
                scene.addComponent<MeshRenderer>(entity, MeshRenderer{AssetRef<Mesh>{node.mesh, ResourceHandle<Mesh>{slot.index, slot.generation}}});
            }
        }
        return true;
    }

    void PlaceModelCommand::removeEntities(Scene& scene, std::vector<Entity>& entities) const {
        for (const Entity entity: entities) {
            if (!scene.contains(entity)) {
                continue;
            }
            const bool destroyed = scene.destroyEntity(entity);
            TE_CHECK(destroyed, "Failed to remove entity {0} of the refused {1}", entity, m_path);
        }
        entities.clear();
    }
}
