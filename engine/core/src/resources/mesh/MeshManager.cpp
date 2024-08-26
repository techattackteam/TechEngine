#include "MeshManager.hpp"


#include "AssimpLoader.hpp"
#include "Mesh.hpp"
#include "serialization/FileStream.hpp"

namespace TechEngine {
    void MeshManager::init() {
        //m_meshesBank.clear();
    }


    void MeshManager::shutdown() {
        m_meshesBank.clear();
    }

    void MeshManager::registerMesh(const std::string& name, AssimpLoader::MeshData& meshData, Material& material) {
        if (isMeshRegistered(name)) {
            return;
        }
        m_meshesBank.emplace_back(name, material, meshData.vertices, meshData.indices);
    }

    const Mesh& MeshManager::getMesh(const std::string& name) {
        for (Mesh& mesh: m_meshesBank) {
            if (mesh.getName() == name) {
                return mesh;
            }
        }
        throw std::runtime_error("Mesh not found");
    }

    bool MeshManager::isMeshRegistered(const std::string& name) {
        return std::ranges::any_of(m_meshesBank, [&name](Mesh& mesh) {
            return mesh.getName() == name;
        });
    }
}
