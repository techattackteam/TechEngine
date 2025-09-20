#include "MeshManager.hpp"

#include "Mesh.hpp"
#include "AssimpLoader.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "systems/SystemsRegistry.hpp"
#include "events/resourcersManager/meshManager/MeshCreatedEvent.hpp"
#include "events/resourcersManager/meshManager/MeshDeletedEvent.hpp"

namespace TechEngine {
    MeshManager::MeshManager(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry) {
    }

    void MeshManager::init(const std::vector<std::filesystem::path>& meshFilePaths) {
        m_meshesBank.reserve(10);
        for (const std::filesystem::path& meshFilePath: meshFilePaths) {
            loadStaticMesh(meshFilePath);
        }
    }

    void MeshManager::shutdown() {
        m_meshesBank.clear();
    }


    void MeshManager::createMesh(const std::string& name) {
        if (isMeshLoaded(name)) {
            return;
        }
        createMesh(name, std::vector<Vertex>(), std::vector<int>());
    }

    void MeshManager::createMesh(const std::string& name, const std::vector<Vertex>& vertices, const std::vector<int>& indices) {
        if (isMeshLoaded(name)) {
            return;
        }
        m_meshesBank.emplace_back(name, vertices, indices);
        m_systemsRegistry.getSystem<EventDispatcher>().dispatch<MeshCreatedEvent>(name);
    }

    void MeshManager::loadStaticMesh(const std::filesystem::path& path) {
        FileStreamReader reader(path);
        AssimpLoader::aiMeshData meshData;
        reader.readObject(meshData);
        std::string name = path.stem().string();
        if (isMeshLoaded(name)) {
            return;
        }
        createMesh(name, meshData.vertices, meshData.indices);
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

    const std::vector<Mesh>& MeshManager::getMeshes() {
        return m_meshesBank;
    }

    void MeshManager::deleteMesh(const std::string& name) {
        Mesh mesh = getMesh(name);
        for (auto it = m_meshesBank.begin(); it != m_meshesBank.end(); ++it) {
            if (it->getName() == name) {
                m_meshesBank.erase(it);
                break;
            }
        }
        m_systemsRegistry.getSystem<EventDispatcher>().dispatch<MeshDeletedEvent>(m_meshesBank.back());
    }
}
