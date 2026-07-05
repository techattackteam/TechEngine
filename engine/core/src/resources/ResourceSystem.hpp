#pragma once

#include <filesystem>

#include "IResourceCache.hpp"
#include "scene/SceneResource.hpp"
#include "TechEngine/core/components/Entity.hpp"
#include "TechEngine/core/resources/model/ModelResource.hpp"
#include "TechEngine/core/resources/texture/TextureResource.hpp"
#include "TechEngine/core/resources/material/MaterialResource.hpp"
#include "TechEngine/core/resources/mesh/MeshResource.hpp"
#include "TechEngine/core/scene/Scene.hpp"

namespace TechEngine {
    enum class AppType;
    class Scene;

    class CORE_DLL ResourceSystem : public System {
    private:
        SystemsRegistry& m_systemsRegistry;

        IResourceCache<ModelResource> m_modelCache = IResourceCache<ModelResource>();
        IResourceCache<MeshResource> m_meshCache = IResourceCache<MeshResource>();
        IResourceCache<MaterialResource> m_materialCache = IResourceCache<MaterialResource>();
        IResourceCache<TextureResource> m_textureCache = IResourceCache<TextureResource>();
        IResourceCache<SceneResource> m_sceneCache = IResourceCache<SceneResource>();

    public:
        explicit ResourceSystem(SystemsRegistry& systemsRegistry);

        void init();

        void shutdown() override;

#pragma region ModelManager
        bool isModelRegistered(const std::string& name) const;

        bool registerModelResource(const std::shared_ptr<ModelResource>& modelResource);

        bool unregisterModelResource(const std::string& name);

        std::shared_ptr<ModelResource> getModelResource(const std::string& name) const;

        std::shared_ptr<ModelResource> getModelResource(const UUID& uuid) const;

        void createEntityFromModelNodeHelper(Scene& scene, Entity parent, const ModelNode& node);
#pragma endregion

#pragma region MeshManager
        Entity createEntityFromModelNode(Scene& scene, Entity parent, const UUID& modelUUID);

        bool isMeshRegistered(const std::string& name) const;

        bool registerMeshResource(const std::shared_ptr<MeshResource>& meshResource);

        bool unregisterMeshResource(const std::string& name);

        std::shared_ptr<MeshResource> getMeshResource(const std::string& name) const;

        std::shared_ptr<MeshResource> getMeshResource(const UUID& uuid) const;

        std::shared_ptr<MeshResource> getDefaultMesh() const;

#pragma endregion

#pragma region MaterialManager

        std::shared_ptr<MaterialResource> createMaterialResource(const std::string& name);

        bool isMaterialRegistered(const std::string& name) const;

        bool registerMaterialResource(const std::shared_ptr<MaterialResource>& materialResource);

        bool unregisterMaterialResource(const std::string& name);

        std::shared_ptr<MaterialResource> getMaterialResource(const std::string& name) const;

        std::shared_ptr<MaterialResource> getMaterialResource(const UUID& uuid) const;

        std::shared_ptr<MaterialResource> getDefaultMaterial() const;

        void assignTextureToMaterial(const std::string& materialName, const std::string& textureName, const std::string& textureType);

#pragma endregion

#pragma region TextureManager

        bool isTextureRegistered(const std::string& name) const;

        bool registerTextureResource(const std::shared_ptr<TextureResource>& texture);

        bool unregisterTextureResource(const std::string& name);

        std::shared_ptr<TextureResource> getTextureResource(const std::string& name) const;

        std::shared_ptr<TextureResource> getTextureResource(const UUID& uuid) const;

#pragma endregion

#pragma region SceneManager
        bool isSceneRegistered(const std::string& name) const;

        bool registerSceneResource(const std::shared_ptr<SceneResource>& sceneResource);

        bool unregisterSceneResource(const std::string& name);

        std::shared_ptr<SceneResource> getSceneResource(const std::string& name) const;

        std::shared_ptr<SceneResource> getSceneResource(const UUID& uuid) const;

        std::vector<std::string> getSceneNames() const;
#pragma endregion

    private:
        // Cascade helpers: reset every referencer of a soon-to-be-deleted resource back to the
        // engine default. All run synchronously on the main thread (ECS + caches are not safe for
        // concurrent structural edits).
        void reassignEntitiesToDefaultMesh(const UUID& oldUUID);

        void reassignEntitiesToDefaultMaterial(const UUID& oldUUID);

        void reassignModelsToDefaultMesh(const UUID& oldUUID);

        void reassignModelsToDefaultMaterial(const UUID& oldUUID);

        void resetMaterialsReferencingTexture(const UUID& oldUUID);
    };
}
