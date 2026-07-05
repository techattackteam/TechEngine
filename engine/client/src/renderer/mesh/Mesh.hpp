#pragma once
#include "renderer/resources/IGPUResource.hpp"

namespace TechEngine {
    class Mesh : public IGPUResource {
    private:
        friend class GpuResourceManager;
        uint32_t m_indexCount = 0;
        uint32_t m_vertexCount = 0;
        uint32_t m_firstIndex = 0;
        uint32_t m_baseVertex = 0;

    public:
        Mesh(const UUID& resourceUUID, uint32_t indexCount, uint32_t vertexCount, uint32_t firstIndex, uint32_t baseVertex);

        uint32_t getIndexCount() const;

        uint32_t getVertexCount() const;

        uint32_t getFirstIndex() const;

        uint32_t getBaseVertex() const;
    };
}
