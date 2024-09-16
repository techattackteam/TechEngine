#pragma once

#include <queue>
#include <yaml-cpp/node/node.h>

#include "systems/System.hpp"
#include "material/MaterialManager.hpp"
#include "mesh/AssimpLoader.hpp"
#include "mesh/MeshManager.hpp"


namespace TechEngine {
    enum class AppType;

    class ResourcesManager : public System {
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
        void loadModelFile(const std::string& path);

        void loadStaticMesh(const std::string& path);

        Mesh& getMesh(const std::string& name);

        Mesh& getDefaultMesh();
#pragma endregion

#pragma region MaterialManager
        void createMaterial(const std::string& name, const std::string& path);

        Material& getMaterial(const std::string& name);

        Material& getDefaultMaterial();
#pragma endregion

    private:
        std::unordered_map<std::string, std::vector<std::filesystem::path>> getFilesByExtension(const AppType& appType);
    };
}
