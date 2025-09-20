#pragma once
#include <cstdint>

namespace TechEngine {
    class VertexBuffer;

    class VertexArray {
    private:
        uint32_t id;

    public:
        VertexArray() = default;

        ~VertexArray();

        void init();

        void addNewBufferOldFormat(const VertexBuffer& vertexBuffer) const;

        void addNewBuffer(const VertexBuffer& vertexBuffer) const;

        void addNewLinesBuffer(const VertexBuffer& vertexBuffer) const;

        void addNewUIBuffer(const VertexBuffer& vertexBuffer) const;

        void bind() const;

        void unBind() const;
    };
}
