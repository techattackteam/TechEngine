#pragma once
#include "Mesh.hpp"


#include <filesystem>
#include <string>

namespace TechEngine {
    class MeshManager {
    private:
        std::vector<Mesh> m_meshesBank;

    public:
        inline static const std::string DEFAULT_MESH_NAME = "Cube";

        void init(const std::vector<std::filesystem::path>& meshFilePaths);

        void shutdown();

        void loadStaticMesh(const std::filesystem::path& path);

        Mesh& getMesh(const std::string& name);

        bool isMeshLoaded(const std::string& name);

    private:
    };
}
