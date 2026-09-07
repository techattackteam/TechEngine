#pragma once

#include <cstddef>
#include <cstdint>

namespace TechEngine {
    class Buffer;

    class VertexArray {
    private:
        unsigned int m_id = 0;

    public:
        VertexArray() = default;

        ~VertexArray();

        VertexArray(const VertexArray&) = delete;

        VertexArray& operator=(const VertexArray&) = delete;

        VertexArray(VertexArray&&) = delete;

        VertexArray& operator=(VertexArray&&) = delete;

        bool initialize();

        void setVertexBuffer(const Buffer& buffer, std::uint32_t binding, std::ptrdiff_t offset, int stride) const;

        void setIndexBuffer(const Buffer& buffer) const;

        void setFloatAttribute(std::uint32_t location, std::uint32_t binding, int componentCount, std::uint32_t relativeOffset) const;

        void bind() const;

        static void unbind();

        void shutdown();
    };
}
