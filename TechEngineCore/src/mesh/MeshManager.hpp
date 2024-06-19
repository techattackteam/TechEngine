#pragma once
#include "Mesh.hpp"

#include <unordered_map>
#include <string>

#include "core/Logger.hpp"

namespace TechEngine {
    class MeshManager {
    private:
        std::unordered_map<std::string, Mesh*> _meshBank = std::unordered_map<std::string, Mesh*>();

    public:
        MeshManager(const MeshManager&) = delete;

        static void init();

        static void registerMesh(const std::string& name, Mesh& mesh);

        static Mesh& getMesh(const std::string& name);

        static bool isMeshRegistered(const std::string& name);

        MeshManager& operator=(const MeshManager&) = delete;

        static MeshManager& getInstance();

    private:
        MeshManager() {
            TE_LOGGER_INFO("MeshManager constructed.");
        };

        void registerMeshImpl(const std::string& name, Mesh& mesh);

        Mesh& getMeshImpl(const std::string& name);

        bool isMeshRegisteredImpl(const std::string& name);
    };
}
