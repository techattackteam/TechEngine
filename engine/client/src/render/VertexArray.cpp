#include <render/Buffer.hpp>
#include <render/VertexArray.hpp>

#include <glad/gl.h>

namespace TechEngine {
    VertexArray::~VertexArray() {
        shutdown();
    }

    bool VertexArray::initialize() {
        if (m_id != 0) {
            return false;
        }

        glCreateVertexArrays(1, &m_id);
        return m_id != 0;
    }

    void VertexArray::setVertexBuffer(const Buffer& buffer, const std::uint32_t binding, const std::ptrdiff_t offset, const int stride) const {
        glVertexArrayVertexBuffer(m_id, binding, buffer.id(), offset, stride);
    }

    void VertexArray::setIndexBuffer(const Buffer& buffer) const {
        glVertexArrayElementBuffer(m_id, buffer.id());
    }

    void VertexArray::setFloatAttribute(const std::uint32_t location, const std::uint32_t binding, const int componentCount, const std::uint32_t relativeOffset) const {
        glEnableVertexArrayAttrib(m_id, location);
        glVertexArrayAttribFormat(m_id, location, componentCount, GL_FLOAT, GL_FALSE, relativeOffset);
        glVertexArrayAttribBinding(m_id, location, binding);
    }

    void VertexArray::bind() const {
        glBindVertexArray(m_id);
    }

    void VertexArray::unbind() {
        glBindVertexArray(0);
    }

    void VertexArray::shutdown() {
        if (m_id == 0) {
            return;
        }

        glDeleteVertexArrays(1, &m_id);
        m_id = 0;
    }
}
