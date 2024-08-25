#include "IndicesBuffer.hpp"
#include "ErrorCatcher.hpp"


namespace TechEngine {
    IndicesBuffer::~IndicesBuffer() {
        GlCall(glDeleteBuffers(1, &this->id));
    }

    void IndicesBuffer::init(uint32_t size) {
        GlCall(glGenBuffers(1, &this->id));
        GlCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->id));
        GlCall(glBufferData(GL_ELEMENT_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW));
    }

    void IndicesBuffer::bind() const {
        GlCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->id));
    }

    void IndicesBuffer::unBind() const {
        GlCall(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0));
    }

    void IndicesBuffer::addData(const void *data, uint32_t size) const {
        bind();
        GlCall(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, size, data));
        unBind();
    }
}