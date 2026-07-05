#include "TechEngine/core/resources/mesh/MeshResource.hpp"

namespace TechEngine {
    MeshResource::MeshResource(const std::string& name,
                               std::vector<Vertex> vertices,
                               std::vector<int> indices) : m_vertices(std::move(vertices)),
                                                           m_indices(std::move(indices)),
                                                           IResource(name) {
    }

    MeshResource::MeshResource(const MeshResource& rhs) : m_vertices(rhs.m_vertices),
                                                          m_indices(rhs.m_indices),
                                                          IResource(rhs.m_name) {
        TE_LOGGER_INFO("MeshResource copy: {0} {1} {2}", m_name, m_vertices.size(), m_indices.size());
    }

    void MeshResource::setVertices(const std::vector<Vertex>& vector, const std::vector<int>& indices) {
        this->m_vertices = vector;
        this->m_indices = indices;
    }

    void MeshResource::addVertices(const std::vector<Vertex>& vector, const std::vector<int>& indices) {
        for (const Vertex& vertex: vector) {
            this->m_vertices.emplace_back(vertex);
        }
        for (int index: indices) {
            this->m_indices.emplace_back(index);
        }
    }

    const std::vector<Vertex>& MeshResource::getVertices() const {
        return m_vertices;
    }

    const std::vector<int>& MeshResource::getIndices() const {
        return m_indices;
    }

    MeshResource::MeshResource() : IResource() {
    }

    void MeshResource::serialize(StreamWriter* writer, const MeshResource& meshResource) {
        IResource::serialize(writer, meshResource);
        writer->writeString(meshResource.m_name);
        writer->writeArray(meshResource.m_vertices);
        writer->writeArray(meshResource.m_indices);
    }

    std::shared_ptr<MeshResource> MeshResource::deserialize(StreamReader* reader) {
        std::shared_ptr<MeshResource> meshResource = std::shared_ptr<MeshResource>(new MeshResource());
        IResource::deserialize(reader, *meshResource);
        reader->readString(meshResource->m_name);
        reader->readArray(meshResource->m_vertices);
        reader->readArray(meshResource->m_indices);
        return meshResource;
    }
}
