#pragma once
#include "Mesh.hpp"
#include "systems/System.hpp"


#include <filesystem>
#include <string>
#include <unordered_map>

#include "AssimpLoader.hpp"

namespace TechEngine {
    class MeshManager {
    private:
        std::vector<Mesh> m_meshesBank;

    public:
        void init();

        void shutdown();

        void registerMesh(const std::string& name, AssimpLoader::MeshData& meshData, Material& material);

        const Mesh& getMesh(const std::string& name);

        bool isMeshRegistered(const std::string& name);
    };
}
