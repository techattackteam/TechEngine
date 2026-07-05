#include "Mesh.hpp"

namespace TechEngine {
    Mesh::Mesh(const UUID& resourceUUID,
               uint32_t indexCount,
               uint32_t vertexCount,
               uint32_t firstIndex,
               uint32_t baseVertex) : IGPUResource(resourceUUID),
                                      m_indexCount(indexCount),
                                      m_vertexCount(vertexCount),
                                      m_firstIndex(firstIndex),
                                      m_baseVertex(baseVertex) {
    }

    uint32_t Mesh::getIndexCount() const {
        return m_indexCount;
    }

    uint32_t Mesh::getVertexCount() const {
        return m_vertexCount;
    }

    uint32_t Mesh::getFirstIndex() const {
        return m_firstIndex;
    }

    uint32_t Mesh::getBaseVertex() const {
        return m_baseVertex;
    }
}
