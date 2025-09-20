#pragma once
#include "Mesh.hpp"


#include <filesystem>
#include <string>

namespace TechEngine {
    class MeshManager {
    private:
        //TODO: Change to unordered_map for faster access
        std::vector<Mesh> m_meshesBank;
        SystemsRegistry& m_systemsRegistry;

    public:
        inline static const std::string DEFAULT_MESH_NAME = "Cube";

        MeshManager(SystemsRegistry& systemsRegistry);

        void init(const std::vector<std::filesystem::path>& meshFilePaths);

        void shutdown();

        void createMesh(const std::string& name);

        void createMesh(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<int>& indices);

        void loadStaticMesh(const std::filesystem::path& path);

        Mesh& getMesh(const std::string& name);

        bool isMeshLoaded(const std::string& name);

        const std::vector<Mesh>& getMeshes();

        void deleteMesh(const std::string& name);

    private:
    };
}
