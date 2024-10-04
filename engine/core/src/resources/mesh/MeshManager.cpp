#include "MeshManager.hpp"

#include "Mesh.hpp"
#include "AssimpLoader.hpp"

namespace TechEngine {
    void MeshManager::init(const std::vector<std::filesystem::path>& meshFilePaths) {
        m_meshesBank.reserve(10);
        for (const std::filesystem::path& meshFilePath: meshFilePaths) {
            loadStaticMesh(meshFilePath);
        }
    }

    void MeshManager::shutdown() {
        //m_meshesBank.clear();
    }


    void MeshManager::createMesh(const std::string& name) {
        if (isMeshLoaded(name)) {
            return;
        }
        m_meshesBank.emplace_back(name, std::vector<Vertex>(), std::vector<int>());
    }

    void MeshManager::createMesh(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<int>& indices) {
        if (isMeshLoaded(name)) {
            return;
        }
        m_meshesBank.emplace_back(name, vertices, indices);
    }

    void MeshManager::loadStaticMesh(const std::filesystem::path& path) {
        FileStreamReader reader(path);
        AssimpLoader::aiMeshData meshData;
        reader.readObject(meshData);
        std::string name = path.stem().string();
        if (isMeshLoaded(name)) {
            return;
        }
        m_meshesBank.emplace_back(name, meshData.vertices, meshData.indices);
    }

    Mesh& MeshManager::getMesh(const std::string& name) {
        for (Mesh& mesh: m_meshesBank) {
            if (mesh.getName() == name) {
                return mesh;
            }
        }
        throw std::runtime_error("Mesh not found");
    }

    bool MeshManager::isMeshLoaded(const std::string& name) {
        return std::ranges::any_of(m_meshesBank, [&name](Mesh& mesh) {
            return mesh.getName() == name;
        });
    }
}
