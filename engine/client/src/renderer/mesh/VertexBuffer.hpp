#pragma once
#include <cstdint>

namespace TechEngine {
    class VertexBuffer {
    private:
        uint32_t m_id;
        uint32_t m_used = 0;
        uint32_t m_size = 0;

    public:
        VertexBuffer() = default;

        ~VertexBuffer();

        void init(uint32_t size);

        void bind() const;

        void unBind() const;

        void addData(const void* data, uint32_t size, uint32_t offset);

        uint32_t appendData(const void* data, uint32_t size);

        void remove(uint32_t byteOffset, uint32_t byteSize);

        void resize(uint32_t newSize);

        uint32_t getOffset();

        void setOffset(int offset);
    };
}
