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

        /**
         * Create an empty mesh with the given name without serializing it
         * @param name
         */
        void createMesh(const std::string& name);

        /**
         * Create a mesh with the given name and vertices and indices
         * @param name
         * @param vertices
         * @param indices
         */
        void createMesh(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<int>& indices);

        void loadStaticMesh(const std::filesystem::path& path);

        Mesh& getMesh(const std::string& name);

        bool isMeshLoaded(const std::string& name);

    private:
    };
}
