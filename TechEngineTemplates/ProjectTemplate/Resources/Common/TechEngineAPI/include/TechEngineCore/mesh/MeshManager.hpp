#pragma once
#include "Mesh.hpp"

#include "core/Logger.hpp"
#include <unordered_map>
#include <string>


namespace TechEngine {
    class CORE_DLL MeshManager : public System {
    private:
        std::unordered_map<std::string, Mesh*> _meshBank = std::unordered_map<std::string, Mesh*>();

    public:
        MeshManager();

        void init();

        void registerMesh(const std::string& name, Mesh& mesh);

        Mesh& getMesh(const std::string& name);

        bool isMeshRegistered(const std::string& name);
    };
}
