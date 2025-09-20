#include "IndicesBuffer.hpp"
#include "ErrorCatcher.hpp"


namespace TechEngine {
    IndicesBuffer::~IndicesBuffer() {
        glDeleteBuffers(1, &this->id);
    }

    void IndicesBuffer::init(uint32_t size) {
        glGenBuffers(1, &this->id);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    }

    void IndicesBuffer::bind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->id);
    }

    void IndicesBuffer::unBind() const {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    void IndicesBuffer::addData(const void* data, uint32_t size, uint32_t offset) const {
        bind();
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, size, data);
        unBind();
    }
}
