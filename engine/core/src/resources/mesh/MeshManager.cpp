#include "MeshManager.hpp"

#include "Mesh.hpp"
#include "AssimpLoader.hpp"

namespace TechEngine {
    void MeshManager::init(const std::vector<std::filesystem::path>& meshFilePaths) {
        for (const std::filesystem::path& meshFilePath: meshFilePaths) {
            loadStaticMesh(meshFilePath);
        }
    }

    void MeshManager::shutdown() {
        m_meshesBank.clear();
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
