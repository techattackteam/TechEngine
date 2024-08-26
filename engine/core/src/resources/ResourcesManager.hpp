#pragma once

#include <queue>

#include "files/FileUtils.hpp"
#include "systems/System.hpp"
#include "material/MaterialManager.hpp"
#include "mesh/AssimpLoader.hpp"
#include "mesh/MeshManager.hpp"


namespace TechEngine {
    class ResourcesManager : public System {
    private:
        MaterialManager m_materialManager;
        MeshManager m_meshManager;
        AssimpLoader m_assimpLoader;

    public:
        void init() override;

        void shutdown() override;

        void loadModelFile(const std::string& path);

        void registerStaticMesh(const std::string& path);

    private:
        std::filesystem::path createStaticMeshFile(const AssimpLoader::Node& node, const std::string& staticMeshPath);

        AssimpLoader::MeshData loadStaticMesh(const std::filesystem::path& path);
    };
}
