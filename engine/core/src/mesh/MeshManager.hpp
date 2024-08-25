#pragma once
#include "Mesh.hpp"
#include "systems/System.hpp"


#include <filesystem>
#include <string>
#include <unordered_map>

namespace TechEngine {
    class MeshManager : public System {
    private:
        std::unordered_map<std::string, Mesh> m_meshesBank;

    public:
        void init() override;

        void shutdown() override;

        void registerMesh(const std::filesystem::path& path);

        const Mesh& getMesh(const std::string& name);

    private:
        Mesh loadStaticMesh(const std::filesystem::path& path);

        bool isMeshRegistered(const std::string& name);
    };
}
