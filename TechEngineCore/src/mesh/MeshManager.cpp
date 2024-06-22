#include "MeshManager.hpp"

#include "CubeMesh.hpp"
#include "CylinderMesh.hpp"
#include "SphereMesh.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    MeshManager::MeshManager() {
    }

    void MeshManager::init() {
        registerMesh("Cube", *new CubeMesh());
        registerMesh("Sphere", *new SphereMesh());
        registerMesh("Cylinder", *new CylinderMesh());
    }

    bool MeshManager::isMeshRegistered(const std::string& name) {
        return _meshBank.find(name) != _meshBank.end();
    }

    void MeshManager::registerMesh(const std::string& name, Mesh& mesh) {
        if (!isMeshRegistered(name)) {
            _meshBank[name] = &mesh;
        } else {
            TE_LOGGER_ERROR("Mesh " + name + " is already registered.");
        }
    }

    Mesh& MeshManager::getMesh(const std::string& name) {
        if (!isMeshRegistered(name)) {
            TE_LOGGER_ERROR("Mesh " + name + " is not registered. Returning Cube mesh.");
            return *_meshBank["Cube"];
        } else {
            return *_meshBank[name];
        }
    }
}
