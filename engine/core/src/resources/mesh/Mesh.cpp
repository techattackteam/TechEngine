#include "TechEngine/core/resources/mesh/Mesh.hpp"
#include "TechEngine/core/resources/mesh/Vertex.hpp"
#include "core/Logger.hpp"
#include "serialization/StreamReader.hpp"
#include "serialization/StreamWriter.hpp"

namespace TechEngine {
    Mesh::Mesh(std::string name,
               std::vector<Vertex> vertices,
               std::vector<int> indices) : m_name(std::move(name)),
                                           m_vertices(std::move(vertices)),
                                           m_indices(std::move(indices)), IResource() {
    }

    Mesh::Mesh(const Mesh& rhs) : m_name(rhs.m_name),
                                  m_vertices(rhs.m_vertices),
                                  m_indices(rhs.m_indices) {
        TE_LOGGER_INFO("Mesh copy: {0} {1} {2}", m_name, m_vertices.size(), m_indices.size());
    }

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

    void Mesh::Serialize(StreamWriter* writer, const Mesh& mesh) {
        writer->writeArray(mesh.m_vertices);
        writer->writeArray(mesh.m_indices);
    }

    void Mesh::Deserialize(StreamReader* reader, Mesh& mesh) {
        reader->readArray(mesh.m_vertices);
        reader->readArray(mesh.m_indices);
    }

    const std::string& Mesh::getName() const {
        return m_name;
    }
}
