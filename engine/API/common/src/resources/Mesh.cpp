#include "resources/Mesh.hpp"

#include "resources/ResourcesManager.hpp"

namespace TechEngineAPI {
    Mesh::Mesh(const std::string& name, TechEngine::ResourcesManager* resourcesManager) : m_name(name), m_resourcesManager(resourcesManager) {
    }

    void Mesh::setVertices(const std::vector<Vertex>& vertices, const std::vector<int>& indices) {
        std::vector<TechEngine::Vertex> ver;
        for (const Vertex& vertex: vertices) {
            ver.emplace_back(vertex.position, vertex.normal, vertex.texCoords, vertex.color);
        }
        m_resourcesManager->setVertices(m_name, ver, indices);
    }

    void Mesh::addVertices(const std::vector<Vertex>& vertices, const std::vector<int>& indices) {
        std::vector<TechEngine::Vertex> ver;
        for (const Vertex& vertex: vertices) {
            ver.emplace_back(vertex.position, vertex.normal, vertex.texCoords, vertex.color);
        }
        m_resourcesManager->addVertices(m_name, ver, indices);
    }

    std::string Mesh::getName() {
        return m_name;
    }

    void Mesh::setName(const std::string& name) {
        this->m_name = name;
        // resourcesManager->updateMesh(this);
    }
}
