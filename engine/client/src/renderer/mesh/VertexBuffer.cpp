#include "VertexBuffer.hpp"
#include "renderer/ErrorCatcher.hpp"

namespace TechEngine {
    VertexBuffer::~VertexBuffer() {
        glDeleteBuffers(1, &m_id);
        m_used = 0;
        m_size = 0;
    }

    void VertexBuffer::init(uint32_t size) {
        glGenBuffers(1, &this->m_id);
        glBindBuffer(GL_ARRAY_BUFFER, this->m_id);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
        this->m_size = size;
        this->m_used = 0;
    }

    void VertexBuffer::bind() const {
        glBindBuffer(GL_ARRAY_BUFFER, this->m_id);
    }

    void VertexBuffer::unBind() const {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void VertexBuffer::addData(const void* data, uint32_t size, uint32_t offset) {
        if (offset + size > m_size) {
            const uint32_t requiredSize = offset + size;
            uint32_t newSize = (m_size == 0) ? requiredSize * 2 : m_size * 2;

            while (requiredSize > newSize) {
                newSize *= 2;
            }

            resize(newSize);
        }
        bind();
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
        unBind();
        if (offset + size > m_used) {
            m_used = offset + size;
        }
    }

    uint32_t VertexBuffer::appendData(const void* data, uint32_t size) {
        if (m_used + size > m_size) {
            const uint32_t requiredSize = m_used + size;
            uint32_t newSize = (m_size == 0) ? size * 2 : m_size * 2;

            while (requiredSize > newSize) {
                newSize *= 2;
            }

            resize(newSize);
        }

        bind();
        const uint32_t offset = m_used;
        glBindBuffer(GL_ARRAY_BUFFER, m_id);
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_used += size;
        unBind();
        return offset;
    }

    void VertexBuffer::remove(uint32_t byteOffset, uint32_t byteSize) {
        // Guard against out-of-range removals: tailSize is unsigned, so byteOffset + byteSize > m_used
        // would underflow into a huge value and blow up the copy below.
        if (byteSize == 0 || byteOffset + byteSize > m_used) {
            return;
        }
        bind();
        const uint32_t tailOffset = byteOffset + byteSize;
        const uint32_t tailSize = m_used - tailOffset;

        if (tailSize > 0) {
            uint32_t tempId = 0;
            glGenBuffers(1, &tempId);
            glBindBuffer(GL_COPY_WRITE_BUFFER, tempId);
            glBufferData(GL_COPY_WRITE_BUFFER, tailSize, nullptr, GL_DYNAMIC_DRAW);

            glBindBuffer(GL_COPY_READ_BUFFER, m_id);
            glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, tailOffset, 0, tailSize);

            glBindBuffer(GL_COPY_READ_BUFFER, tempId);
            glBindBuffer(GL_COPY_WRITE_BUFFER, m_id);
            glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, byteOffset, tailSize);

            glDeleteBuffers(1, &tempId);
            glBindBuffer(GL_COPY_READ_BUFFER, 0);
            glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
        }

        m_used -= byteSize;
        unBind();
    }

    void VertexBuffer::resize(uint32_t newSize) {
        if (newSize <= m_size) {
            return;
        }

        uint32_t newId = 0;
        glGenBuffers(1, &newId);
        glBindBuffer(GL_COPY_WRITE_BUFFER, newId);
        glBufferData(GL_COPY_WRITE_BUFFER, newSize, nullptr, GL_DYNAMIC_DRAW);

        if (m_used > 0) {
            glBindBuffer(GL_COPY_READ_BUFFER, m_id);
            glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, m_used);
            glBindBuffer(GL_COPY_READ_BUFFER, 0);
        }

        glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
        glDeleteBuffers(1, &m_id);
        m_id = newId;
        m_size = newSize;
    }
}
