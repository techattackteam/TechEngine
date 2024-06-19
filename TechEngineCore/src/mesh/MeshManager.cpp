#include "MeshManager.hpp"

#include "CubeMesh.hpp"
#include "CylinderMesh.hpp"
#include "SphereMesh.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    void MeshManager::init() {
        registerMesh("Cube", *new CubeMesh());
        registerMesh("Sphere", *new SphereMesh());
        registerMesh("Cylinder", *new CylinderMesh());
    }

    void MeshManager::registerMesh(const std::string& name, Mesh& mesh) {
        getInstance().registerMeshImpl(name, mesh);
    }

    Mesh& MeshManager::getMesh(const std::string& name) {
        return getInstance().getMeshImpl(name);
    }

    bool MeshManager::isMeshRegistered(const std::string& name) {
        return getInstance().isMeshRegisteredImpl(name);
    }

    MeshManager& MeshManager::getInstance() {
        static MeshManager instance;
        return instance;
    }

    bool MeshManager::isMeshRegisteredImpl(const std::string& name) {
        return _meshBank.find(name) != _meshBank.end();
    }

    void MeshManager::registerMeshImpl(const std::string& name, Mesh& mesh) {
        if (!isMeshRegistered(name)) {
            _meshBank[name] = &mesh;
        } else {
            TE_LOGGER_ERROR("Mesh " + name + " is already registered.");
        }
    }

    Mesh& MeshManager::getMeshImpl(const std::string& name) {
        if (!isMeshRegisteredImpl(name)) {
            TE_LOGGER_ERROR("Mesh " + name + " is not registered. Returning Cube mesh.");
            return *_meshBank["Cube"];
        } else {
            return *_meshBank[name];
        }
    }
}
