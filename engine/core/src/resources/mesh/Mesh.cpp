#include "Mesh.hpp"

namespace TechEngine {
    void Mesh::setVertices(const std::vector<Vertex>& vector, const std::vector<int>& indices) {
        this->m_vertices = vector;
        this->m_indices = indices;
    }

    void Mesh::addVertices(const std::vector<Vertex>& vector, const std::vector<int>& indices) {
        for (const Vertex& vertex: vector) {
            this->m_vertices.emplace_back(vertex);
        }
        for (int index: indices) {
            this->m_indices.emplace_back(index);
        }
    }

    const std::string& Mesh::getName() const {
        return m_name;
    }
}
