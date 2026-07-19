#include "ResourceSystem.hpp"
#include "events/resourcersManager/materials/MaterialCreatedEvent.hpp"
#include "events/resourcersManager/materials/MaterialDeletedEvent.hpp"
#include "events/resourcersManager/model/ModelCreatedEvent.hpp"
#include "events/resourcersManager/model/ModelDeletedEvent.hpp"
#include "events/resourcersManager/mesh/MeshCreatedEvent.hpp"
#include "events/resourcersManager/mesh/MeshDeletedEvent.hpp"
#include "events/resourcersManager/texture/TextureCreatedEvent.hpp"
#include "events/resourcersManager/texture/TextureDeletedEvent.hpp"
#include "events/resourcersManager/shader/ShaderCreatedEvent.hpp"
#include "events/resourcersManager/shader/ShaderDeletedEvent.hpp"
#include "systems/SystemsRegistry.hpp"
#include "scene/SceneManager.hpp"
#include "TechEngine/core/scene/Scene.hpp"
#include "eventSystem/EventManager.hpp"

#include <unordered_set>

namespace TechEngine {
    ResourceSystem::ResourceSystem(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    void ResourceSystem::init() {
    }

    void ResourceSystem::shutdown() {
    }

#pragma region ModelManager
    bool ResourceSystem::isModelRegistered(const std::string& name) const {
        return m_modelCache.contains(name);
    }

    bool ResourceSystem::registerModelResource(const std::shared_ptr<ModelResource>& modelResource) {
        if (!modelResource) {
            return false;
        }
        if (m_modelCache.contains(modelResource->getUUID())) {
            const std::shared_ptr<ModelResource> existing = m_modelCache.get(modelResource->getUUID());
            if (existing && existing->getName() == modelResource->getName()) {
                return true; // Same resource already registered.
            }
            TE_LOGGER_ERROR("Model UUID collision: ID {0} is already used by '{1}', cannot register '{2}'",
                            modelResource->getUUID().toString(),
                            existing ? existing->getName() : "<unknown>",
                            modelResource->getName());
            return false;
        }
        if (m_modelCache.contains(modelResource->getName())) {
            TE_LOGGER_WARN("Model already registered: {0}", modelResource->getName());
            return false;
        }
        m_modelCache.add(modelResource);
        m_systemsRegistry.getSystem<EventManager>().dispatch<ModelCreatedEvent>(modelResource->getUUID());
        TE_LOGGER_INFO("Registered Model: {0} (ID: {1})", modelResource->getName(), modelResource->getUUID().toString());
        return true;
    }

    bool ResourceSystem::unregisterModelResource(const std::string& name) {
        const std::shared_ptr<ModelResource> modelResource = m_modelCache.get(name);
        if (!modelResource) {
            TE_LOGGER_WARN("Model not found: {0}", name);
            return false;
        }

        const UUID modelUUID = modelResource->getUUID();

        // Gather the model's dependencies up front (mesh/material UUIDs, plus the textures the
        // materials reference) before touching any cache, de-duplicated so shared references are
        // only unregistered once.
        std::unordered_set<UUID> meshUUIDs;
        for (const UUID& meshUUID: modelResource->getMeshesUUIDs()) {
            if (!meshUUID.isNull()) {
                meshUUIDs.insert(meshUUID);
            }
        }
        std::unordered_set<UUID> materialUUIDs;
        for (const UUID& materialUUID: modelResource->getMaterialUUIDs()) {
            if (!materialUUID.isNull()) {
                materialUUIDs.insert(materialUUID);
            }
        }
        std::unordered_set<UUID> textureUUIDs;
        for (const UUID& materialUUID: materialUUIDs) {
            const std::shared_ptr<MaterialResource> material = m_materialCache.get(materialUUID);
            if (!material) {
                continue;
            }
            const UUID maps[] = {
                material->getAlbedoMapUUID(), material->getNormalMapUUID(),
                material->getMetallicMapUUID(), material->getRoughnessMapUUID(),
                material->getAmbientOcclusionMapUUID(), material->getEmissionMapUUID()
            };
            for (const UUID& map: maps) {
                if (!map.isNull()) {
                    textureUUIDs.insert(map);
                }
            }
        }

        // Remove the model first so it is not seen by the referencer scans below.
        m_modelCache.remove(modelUUID);
        m_systemsRegistry.getSystem<EventManager>().dispatch<ModelDeletedEvent>(modelUUID);
        TE_LOGGER_INFO("Unregistered model: {0}", name);

        const std::shared_ptr<MeshResource> defaultMesh = getDefaultMesh();
        const std::shared_ptr<MaterialResource> defaultMaterial = getDefaultMaterial();
        const UUID defaultMeshUUID = defaultMesh ? defaultMesh->getUUID() : UUID(static_cast<uint64_t>(-1));
        const UUID defaultMaterialUUID = defaultMaterial ? defaultMaterial->getUUID() : UUID(static_cast<uint64_t>(-1));

        for (const UUID& meshUUID: meshUUIDs) {
            if (meshUUID == defaultMeshUUID) {
                continue;
            }
            const std::shared_ptr<MeshResource> mesh = m_meshCache.get(meshUUID);
            if (mesh) {
                unregisterMeshResource(mesh->getName());
            }
        }
        for (const UUID& materialUUID: materialUUIDs) {
            if (materialUUID == defaultMaterialUUID) {
                continue;
            }
            const std::shared_ptr<MaterialResource> material = m_materialCache.get(materialUUID);
            if (material) {
                unregisterMaterialResource(material->getName());
            }
        }
        for (const UUID& textureUUID: textureUUIDs) {
            const std::shared_ptr<TextureResource> texture = m_textureCache.get(textureUUID);
            if (texture) {
                unregisterTextureResource(texture->getName());
            }
        }
        return true;
    }

    std::shared_ptr<ModelResource> ResourceSystem::getModelResource(const std::string& name) const {
        if (m_modelCache.contains(name)) {
            return m_modelCache.get(name);
        }
        TE_LOGGER_WARN("Model not found: {0}", name);
        return nullptr;
    }

    std::shared_ptr<ModelResource> ResourceSystem::getModelResource(const UUID& uuid) const {
        if (m_modelCache.contains(uuid)) {
            return m_modelCache.get(uuid);
        }
        TE_LOGGER_WARN("Model not found with ID: {0}", uuid.toString());
        return nullptr;
    }

    void ResourceSystem::createEntityFromModelNodeHelper(Scene& scene, Entity parent, const ModelNode& node) {
        for (size_t i = 0; i < node.children.size(); ++i) {
            const ModelNode& child = node.children[i];

            std::string entityName = child.hasMesh() ? child.name : "TransformNode_" + std::to_string(i);
            Entity nodeEntity = scene.createEntity(entityName);

            Transform& transform = scene.getComponent<Transform>(nodeEntity);
            transform.m_position = child.position;
            transform.m_rotation = glm::eulerAngles(child.rotation);
            transform.m_scale = child.scale;
            transform.m_isDirty = true;

            scene.setParent(parent, nodeEntity);

            if (child.hasMesh()) {
                if (getMeshResource(child.meshUUID)) {
                    MeshRenderer meshRenderer;
                    meshRenderer.meshUUID = child.meshUUID;

                    if (!child.sourceMaterialUUID.isNull() && m_materialCache.contains(child.sourceMaterialUUID)) {
                        meshRenderer.materialUUID = child.sourceMaterialUUID;
                    } else {
                        meshRenderer.materialUUID = getDefaultMaterial()->getUUID();
                    }

                    scene.addComponent(nodeEntity, meshRenderer);
                } else {
                    TE_LOGGER_CRITICAL("Valid Mesh requested but not found in cache. ID: {0}", child.meshUUID.toString());
                }
            }

            createEntityFromModelNodeHelper(scene, nodeEntity, child);
        }
    }

    Entity ResourceSystem::createEntityFromModelNode(Scene& scene, Entity parent, const UUID& modelUUID) {
        const std::shared_ptr<ModelResource> modelResource = getModelResource(modelUUID);
        if (!modelResource) {
            TE_LOGGER_ERROR("Model resource not found with ID: {0}", modelUUID.toString());
            return static_cast<Entity>(-1);
        }
        std::string entityName = modelResource->getName();
        Entity entity = scene.createEntity(entityName);

        Transform& transform = scene.getComponent<Transform>(entity);
        transform.translateTo(modelResource->getRootNode().position);
        transform.setScale(modelResource->getRootNode().scale);
        transform.setRotation(modelResource->getRootNode().rotation);
        transform.m_isDirty = true;

        if (parent != static_cast<Entity>(-1)) {
            scene.setParent(parent, entity);
        }

        createEntityFromModelNodeHelper(scene, entity, modelResource->getRootNode());
        return entity;
    }

#pragma endregion

#pragma region MeshManager
    bool ResourceSystem::isMeshRegistered(const std::string& name) const {
        return m_meshCache.contains(name);
    }

    bool ResourceSystem::registerMeshResource(const std::shared_ptr<MeshResource>& meshResource) {
        if (!meshResource) {
            return false;
        }
        if (m_meshCache.contains(meshResource->getUUID())) {
            const std::shared_ptr<MeshResource> existing = m_meshCache.get(meshResource->getUUID());
            if (existing && existing->getName() == meshResource->getName()) {
                return true; // Same resource already registered.
            }
            TE_LOGGER_ERROR("Mesh UUID collision: ID {0} is already used by '{1}', cannot register '{2}'",
                            meshResource->getUUID().toString(),
                            existing ? existing->getName() : "<unknown>",
                            meshResource->getName());
            return false;
        }
        if (isMeshRegistered(meshResource->getName())) {
            TE_LOGGER_WARN("Mesh already registered: {0}", meshResource->getName());
            return false;
        }
        m_meshCache.add(meshResource);
        m_systemsRegistry.getSystem<EventManager>().dispatch<MeshCreatedEvent>(meshResource->getUUID());
        TE_LOGGER_INFO("Registered mesh: {0} (ID: {1})", meshResource->getName(), meshResource->getUUID().toString());
        return true;
    }

    bool ResourceSystem::unregisterMeshResource(const std::string& name) {
        const std::shared_ptr<MeshResource> meshResource = m_meshCache.get(name);
        if (!meshResource) {
            TE_LOGGER_WARN("Mesh not found: {0}", name);
            return false;
        }
        const UUID meshUUID = meshResource->getUUID();
        reassignEntitiesToDefaultMesh(meshUUID);
        reassignModelsToDefaultMesh(meshUUID);
        m_meshCache.remove(meshUUID);
        m_systemsRegistry.getSystem<EventManager>().dispatch<MeshDeletedEvent>(meshUUID);
        TE_LOGGER_INFO("Unregistered mesh: {0}", name);
        return true;
    }

    std::shared_ptr<MeshResource> ResourceSystem::getMeshResource(const std::string& name) const {
        if (m_meshCache.contains(name)) {
            return m_meshCache.get(name);
        }
        TE_LOGGER_WARN("Mesh not found: {0}", name);
        return getDefaultMesh();
    }

    std::shared_ptr<MeshResource> ResourceSystem::getMeshResource(const UUID& uuid) const {
        if (m_meshCache.contains(uuid)) {
            return m_meshCache.get(uuid);
        }
        TE_LOGGER_WARN("Mesh not found with ID: {0}", uuid.toString());
        return getDefaultMesh();
    }

    std::shared_ptr<MeshResource> ResourceSystem::getDefaultMesh() const {
        return m_meshCache.get("Cube");
    }

#pragma endregion

#pragma region MaterialManager

    bool ResourceSystem::isMaterialRegistered(const std::string& name) const {
        return m_materialCache.contains(name);
    }

    std::shared_ptr<MaterialResource> ResourceSystem::createMaterialResource(const std::string& name) {
        std::shared_ptr<MaterialResource> materialResource = std::make_shared<MaterialResource>(name);
        if (!registerMaterialResource(materialResource)) {
            return nullptr;
        }
        return materialResource;
    }

    bool ResourceSystem::registerMaterialResource(const std::shared_ptr<MaterialResource>& materialResource) {
        if (!materialResource) {
            return false;
        }
        if (m_materialCache.contains(materialResource->getUUID())) {
            const std::shared_ptr<MaterialResource> existing = m_materialCache.get(materialResource->getUUID());
            if (existing && existing->getName() == materialResource->getName()) {
                return true; // Same resource already registered.
            }
            TE_LOGGER_ERROR("Material UUID collision: ID {0} is already used by '{1}', cannot register '{2}'",
                            materialResource->getUUID().toString(),
                            existing ? existing->getName() : "<unknown>",
                            materialResource->getName());
            return false;
        }
        if (m_materialCache.contains(materialResource->getName())) {
            TE_LOGGER_WARN("Material resource already registered: {0}", materialResource->getName());
            return false;
        }
        m_materialCache.add(materialResource);
        TE_LOGGER_INFO("Registered material resource: {0} (ID: {1})", materialResource->getName(), materialResource->getUUID().toString());
        m_systemsRegistry.getSystem<EventManager>().dispatch<MaterialCreatedEvent>(materialResource->getUUID());
        return true;
    }

    bool ResourceSystem::unregisterMaterialResource(const std::string& name) {
        const std::shared_ptr<MaterialResource> materialResource = m_materialCache.get(name);
        if (!materialResource) {
            TE_LOGGER_WARN("Material resource not found: {0}", name);
            return false;
        }
        const UUID materialUUID = materialResource->getUUID();
        reassignEntitiesToDefaultMaterial(materialUUID);
        reassignModelsToDefaultMaterial(materialUUID);
        m_materialCache.remove(materialUUID);
        m_systemsRegistry.getSystem<EventManager>().dispatch<MaterialDeletedEvent>(materialUUID);
        TE_LOGGER_INFO("Unregistered material resource: {0}", name);
        return true;
    }

    std::shared_ptr<MaterialResource> ResourceSystem::getMaterialResource(const std::string& name) const {
        if (m_materialCache.contains(name)) {
            return m_materialCache.get(name);
        }
        TE_LOGGER_WARN("Material resource not found with Name: {0}", name);
        return getDefaultMaterial();
    }

    std::shared_ptr<MaterialResource> ResourceSystem::getMaterialResource(const UUID& uuid) const {
        if (m_materialCache.contains(uuid)) {
            return m_materialCache.get(uuid);
        }
        TE_LOGGER_WARN("Material resource not found with ID: {0}", uuid.toString());
        return getDefaultMaterial();
    }

    std::shared_ptr<MaterialResource> ResourceSystem::getDefaultMaterial() const {
        return m_materialCache.get("DefaultMaterial");
    }


    void ResourceSystem::assignTextureToMaterial(const std::string& materialName, const std::string& textureName, const std::string& textureType) {
        if (!m_materialCache.contains(materialName)) {
            TE_LOGGER_WARN("Material resource not found: {0}", materialName);
            return;
        }
        if (!m_textureCache.contains(textureName)) {
            TE_LOGGER_WARN("Texture resource not found: {0}", textureName);
            return;
        }
        std::shared_ptr<MaterialResource> material = m_materialCache.get(materialName);
        std::weak_ptr texture = m_textureCache.get(textureName);
        std::shared_ptr<TextureResource> txt = texture.lock();
        if (!txt) {
            TE_LOGGER_WARN("Texture resource expired: {0}", textureName);
            return;
        }
        if (textureType == "albedo") {
            material->setAlbedoMapID(txt->getUUID());
        } else if (textureType == "normal") {
            material->setNormalMapID(txt->getUUID());
        } else if (textureType == "metallic") {
            material->setMetallicMapID(txt->getUUID());
        } else if (textureType == "roughness") {
            material->setRoughnessMapID(txt->getUUID());
        } else if (textureType == "ambient occlusion") {
            material->setAmbientOcclusionMapID(txt->getUUID());
        } else {
            TE_LOGGER_WARN("Unknown texture type: {0}", textureType);
        }
    }


#pragma endregion

#pragma region TextureManager
    bool ResourceSystem::isTextureRegistered(const std::string& name) const {
        return m_textureCache.contains(name);
    }

    bool ResourceSystem::registerTextureResource(const std::shared_ptr<TextureResource>& texture) {
        if (!texture) {
            return false;
        }
        if (m_textureCache.contains(texture->getUUID())) {
            const std::shared_ptr<TextureResource> existing = m_textureCache.get(texture->getUUID());
            if (existing && existing->getName() == texture->getName()) {
                return true; // Same resource already registered.
            }
            TE_LOGGER_ERROR("Texture UUID collision: ID {0} is already used by '{1}', cannot register '{2}'",
                            texture->getUUID().toString(),
                            existing ? existing->getName() : "<unknown>",
                            texture->getName());
            return false;
        }
        if (m_textureCache.contains(texture->getName())) {
            TE_LOGGER_WARN("Texture resource already registered: {0}", texture->getName());
            return false;
        }
        m_textureCache.add(texture);
        m_systemsRegistry.getSystem<EventManager>().dispatch<TextureCreatedEvent>(texture->getUUID());
        TE_LOGGER_INFO("Registered resource texture: {0} (ID: {1})", texture->getName(), texture->getUUID().toString());
        return true;
    }

    bool ResourceSystem::unregisterTextureResource(const std::string& name) {
        const std::shared_ptr<TextureResource> textureResource = m_textureCache.get(name);
        if (!textureResource) {
            TE_LOGGER_WARN("Texture resource not found: {0}", name);
            return false;
        }
        const UUID textureUUID = textureResource->getUUID();
        resetMaterialsReferencingTexture(textureUUID);
        m_textureCache.remove(textureUUID);
        m_systemsRegistry.getSystem<EventManager>().dispatch<TextureDeletedEvent>(textureUUID);
        TE_LOGGER_INFO("Unregistered texture resource: {0}", name);
        return true;
    }

    std::shared_ptr<TextureResource> ResourceSystem::getTextureResource(const std::string& name) const {
        if (m_textureCache.contains(name)) {
            return m_textureCache.get(name);
        }
        TE_LOGGER_WARN("Texture resource not found: {0}", name);
        return nullptr;
    }

    std::shared_ptr<TextureResource> ResourceSystem::getTextureResource(const UUID& uuid) const {
        if (m_textureCache.contains(uuid)) {
            return m_textureCache.get(uuid);
        }
        TE_LOGGER_WARN("Texture resource not found with ID: {0}", uuid.toString());
        return nullptr;
    }

#pragma endregion

#pragma region ShaderManager
    bool ResourceSystem::isShaderRegistered(const std::string& name) const {
        return m_shaderCache.contains(name);
    }

    bool ResourceSystem::registerShaderResource(const std::shared_ptr<ShaderResource>& shaderResource) {
        if (!shaderResource) {
            return false;
        }
        if (m_shaderCache.contains(shaderResource->getUUID())) {
            const std::shared_ptr<ShaderResource> existing = m_shaderCache.get(shaderResource->getUUID());
            if (existing && existing->getName() == shaderResource->getName()) {
                return true; // Same resource already registered.
            }
            TE_LOGGER_ERROR("Shader UUID collision: ID {0} is already used by '{1}', cannot register '{2}'",
                            shaderResource->getUUID().toString(),
                            existing ? existing->getName() : "<unknown>",
                            shaderResource->getName());
            return false;
        }
        if (m_shaderCache.contains(shaderResource->getName())) {
            TE_LOGGER_WARN("Shader resource already registered: {0}", shaderResource->getName());
            return false;
        }
        m_shaderCache.add(shaderResource);
        m_systemsRegistry.getSystem<EventManager>().dispatch<ShaderCreatedEvent>(shaderResource->getUUID());
        TE_LOGGER_INFO("Registered shader resource: {0} (ID: {1})", shaderResource->getName(), shaderResource->getUUID().toString());
        return true;
    }

    bool ResourceSystem::unregisterShaderResource(const std::string& name) {
        const std::shared_ptr<ShaderResource> shaderResource = m_shaderCache.get(name);
        if (!shaderResource) {
            TE_LOGGER_WARN("Shader resource not found: {0}", name);
            return false;
        }
        const UUID shaderUUID = shaderResource->getUUID();
        m_shaderCache.remove(shaderUUID);
        m_systemsRegistry.getSystem<EventManager>().dispatch<ShaderDeletedEvent>(shaderUUID);
        TE_LOGGER_INFO("Unregistered shader resource: {0}", name);
        return true;
    }

    std::shared_ptr<ShaderResource> ResourceSystem::getShaderResource(const std::string& name) const {
        if (m_shaderCache.contains(name)) {
            return m_shaderCache.get(name);
        }
        TE_LOGGER_WARN("Shader resource not found: {0}", name);
        return nullptr;
    }

    std::shared_ptr<ShaderResource> ResourceSystem::getShaderResource(const UUID& uuid) const {
        if (m_shaderCache.contains(uuid)) {
            return m_shaderCache.get(uuid);
        }
        TE_LOGGER_WARN("Shader resource not found with ID: {0}", uuid.toString());
        return nullptr;
    }

#pragma endregion

#pragma region SceneManager

    bool ResourceSystem::isSceneRegistered(const std::string& name) const {
        return m_sceneCache.contains(name);
    }

    bool ResourceSystem::registerSceneResource(const std::shared_ptr<SceneResource>& sceneResource) {
        if (!sceneResource) {
            return false;
        }
        if (m_sceneCache.contains(sceneResource->getUUID())) {
            const std::shared_ptr<SceneResource> existing = m_sceneCache.get(sceneResource->getUUID());
            if (existing && existing->getName() == sceneResource->getName()) {
                return true; // Same resource already registered.
            }
            TE_LOGGER_ERROR("Scene UUID collision: ID {0} is already used by '{1}', cannot register '{2}'",
                            sceneResource->getUUID().toString(),
                            existing ? existing->getName() : "<unknown>",
                            sceneResource->getName());
            return false;
        }
        if (m_sceneCache.contains(sceneResource->getName())) {
            TE_LOGGER_WARN("Scene resource already registered: {0}", sceneResource->getName());
            return false;
        }
        m_sceneCache.add(sceneResource);
        TE_LOGGER_INFO("Registered scene resource: {0} (Path: {1})", sceneResource->getName(), sceneResource->getVirtualPath().string());
        return true;
    }

    bool ResourceSystem::unregisterSceneResource(const std::string& name) {
        if (!m_sceneCache.contains(name)) {
            TE_LOGGER_WARN("Scene resource not found: {0}", name);
            return false;
        } else {
            m_sceneCache.remove(name);
            TE_LOGGER_INFO("Unregistered scene resource: {0}", name);
        }
        return true;
    }

    std::shared_ptr<SceneResource> ResourceSystem::getSceneResource(const std::string& name) const {
        if (m_sceneCache.contains(name)) {
            return m_sceneCache.get(name);
        }
        TE_LOGGER_WARN("Scene resource not found with Name: {0}", name);
        return nullptr;
    }

    std::shared_ptr<SceneResource> ResourceSystem::getSceneResource(const UUID& uuid) const {
        if (m_sceneCache.contains(uuid)) {
            return m_sceneCache.get(uuid);
        }
        TE_LOGGER_WARN("Scene resource not found with ID: {0}", uuid.toString());
        return nullptr;
    }

    std::vector<std::string> ResourceSystem::getSceneNames() const {
        return m_sceneCache.getNames();
    }

#pragma endregion

    void ResourceSystem::reassignEntitiesToDefaultMesh(const UUID& oldUUID) {
        if (!m_systemsRegistry.hasSystem<SceneManager>()) {
            return; // Non-graphics registry: no ECS scene to reassign.
        }
        const std::shared_ptr<MeshResource> defaultMesh = getDefaultMesh();
        if (!defaultMesh) {
            return;
        }
        const UUID defaultUUID = defaultMesh->getUUID();
        Scene& scene = m_systemsRegistry.getSystem<SceneManager>().getActiveScene();
        scene.runSystem<MeshRenderer>([&](MeshRenderer& meshRenderer) {
            if (meshRenderer.meshUUID == oldUUID) {
                meshRenderer.meshUUID = defaultUUID;
            }
        });
    }

    void ResourceSystem::reassignEntitiesToDefaultMaterial(const UUID& oldUUID) {
        if (!m_systemsRegistry.hasSystem<SceneManager>()) {
            return;
        }
        const std::shared_ptr<MaterialResource> defaultMaterial = getDefaultMaterial();
        if (!defaultMaterial) {
            return;
        }
        const UUID defaultUUID = defaultMaterial->getUUID();
        Scene& scene = m_systemsRegistry.getSystem<SceneManager>().getActiveScene();
        scene.runSystem<MeshRenderer>([&](MeshRenderer& meshRenderer) {
            if (meshRenderer.materialUUID == oldUUID) {
                meshRenderer.materialUUID = defaultUUID;
            }
        });
    }

    void ResourceSystem::reassignModelsToDefaultMesh(const UUID& oldUUID) {
        const std::shared_ptr<MeshResource> defaultMesh = getDefaultMesh();
        const UUID defaultUUID = defaultMesh ? defaultMesh->getUUID() : UUID(static_cast<uint64_t>(-1));
        for (const std::shared_ptr<ModelResource>& model: m_modelCache.getAll()) {
            model->reassignMesh(oldUUID, defaultUUID);
        }
    }

    void ResourceSystem::reassignModelsToDefaultMaterial(const UUID& oldUUID) {
        const std::shared_ptr<MaterialResource> defaultMaterial = getDefaultMaterial();
        const UUID defaultUUID = defaultMaterial ? defaultMaterial->getUUID() : UUID(static_cast<uint64_t>(-1));
        for (const std::shared_ptr<ModelResource>& model: m_modelCache.getAll()) {
            model->reassignMaterial(oldUUID, defaultUUID);
        }
    }

    void ResourceSystem::resetMaterialsReferencingTexture(const UUID& oldUUID) {
        const UUID nullUUID(static_cast<uint64_t>(-1));
        for (const std::shared_ptr<MaterialResource>& material: m_materialCache.getAll()) {
            if (material->getAlbedoMapUUID() == oldUUID) {
                material->setAlbedoMapID(nullUUID);
            }
            if (material->getNormalMapUUID() == oldUUID) {
                material->setNormalMapID(nullUUID);
            }
            if (material->getMetallicMapUUID() == oldUUID) {
                material->setMetallicMapID(nullUUID);
            }
            if (material->getRoughnessMapUUID() == oldUUID) {
                material->setRoughnessMapID(nullUUID);
            }
            if (material->getAmbientOcclusionMapUUID() == oldUUID) {
                material->setAmbientOcclusionMapID(nullUUID);
            }
            if (material->getEmissionMapUUID() == oldUUID) {
                material->setEmissionMapID(nullUUID);
            }
        }
    }
}
