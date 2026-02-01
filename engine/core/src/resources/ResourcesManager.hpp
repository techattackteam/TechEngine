#pragma once

#include "TechEngine/core/resources/material/MaterialManager.hpp"
#include "TechEngine/core/resources/mesh/MeshManager.hpp"
#include "TechEngine/core/resources/texture/TextureManager.hpp"
#include "mesh/AssimpLoader.hpp"
#include "TechEngine/core/components/Entity.hpp"

namespace TechEngine {
    enum class AppType;
    class Scene;

    class CORE_DLL ResourcesManager : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        AssimpLoader m_assimpLoader;

    public:
        MeshManager m_meshManager;
        MaterialManager m_materialManager;
        TextureManager m_textureManager;

        explicit ResourcesManager(SystemsRegistry& systemsRegistry);

        void init(AppType appType);

        void shutdown() override;

#pragma region MeshManager
        void createMesh(const std::string& name);

        void createMeshFromModelFile(const std::string& path);

        void createModelFromFile(const std::string& path);

        void loadModel(const std::string& path, Scene& scene);

        void loadStaticMesh(const std::string& path);

        void setVertices(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<int>& indices);

        void addVertices(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<int>& indices);

        void deleteMesh(const std::string& name);

        Mesh& getMesh(const std::string& name);

        Mesh& getDefaultMesh();

        const std::vector<Mesh>& getAllMeshes();
#pragma endregion

#pragma region MaterialManager
        void loadMaterial(const std::string& name, const std::string& path);

        Material& createMaterial(const std::string& name);

        Material& getMaterial(const std::string& name);

        Material& getDefaultMaterial();

        void assignTextureToMaterial(const std::string& materialName, const std::string& textureName, const std::string& textureType);

        std::vector<Material*> getAllMaterials();
#pragma endregion

#pragma region TextureManager
        void loadTexture(const std::string& name, const std::string& path);

        TextureResource& getTexture(const std::string& name);

        TextureResource& getTexture(const int id);

        std::vector<TextureResource*> getAllTextures();

        std::vector<TextureResource*> getAllTexturesOfType(const TextureType& type);
#pragma endregion

    private:
        std::unordered_map<std::string, std::vector<std::filesystem::path>> getFilesByExtension(const AppType& appType);

        void registerMeshesFromNode(const AssimpLoader::Node& node, const std::string& modelName, const std::string& parentPath = "");

        Entity createEntityFromNode(Scene& scene, const AssimpLoader::Node& node, Entity parent, const std::string& modelName, const std::string& nodePath = "");

        // Loads all meshes referenced by a ModelNode hierarchy
        void loadMeshesFromModelNode(const AssimpLoader::ModelNode& node, const std::string& modelFolder);

        // Creates entity hierarchy from a .temodel ModelNode
        Entity createEntityFromModelNode(Scene& scene, const AssimpLoader::ModelNode& node, Entity parent, const std::string& modelFolder);
    };
}
