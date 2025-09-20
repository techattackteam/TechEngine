#pragma once

#include "material/MaterialManager.hpp"
#include "mesh/AssimpLoader.hpp"
#include "mesh/MeshManager.hpp"

namespace TechEngine {
    enum class AppType;

    class CORE_DLL ResourcesManager : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        MaterialManager m_materialManager;
        MeshManager m_meshManager;
        AssimpLoader m_assimpLoader;

    public:
        explicit ResourcesManager(SystemsRegistry& systemsRegistry);

        void init(AppType appType);

        void shutdown() override;

#pragma region MeshManager
        void createMesh(const std::string& name);

        void loadModelFile(const std::string& path);

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

        const std::vector<Material*>& getAllMaterials();
#pragma endregion

    private:
        std::unordered_map<std::string, std::vector<std::filesystem::path>> getFilesByExtension(const AppType& appType);
    };
}
