#include "VertexBuffer.hpp"
#include "ErrorCatcher.hpp"

namespace TechEngine {
    VertexBuffer::~VertexBuffer() {
        glDeleteBuffers(1, &id);
    }

    void VertexBuffer::init(uint32_t size) {
        glGenBuffers(1, &this->id);
        glBindBuffer(GL_ARRAY_BUFFER, this->id);
        glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    }

    void VertexBuffer::bind() const {
        glBindBuffer(GL_ARRAY_BUFFER, this->id);
    }

    void VertexBuffer::unBind() const {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    void VertexBuffer::addData(const void* data, uint32_t size, uint32_t offset) const {
        bind();
        glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
        unBind();
    }

    uint32_t VertexBuffer::getOffset() {
        return offset;
    }

    void VertexBuffer::setOffset(int offset) {
        this->offset = offset;
    }
}
