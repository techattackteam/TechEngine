
#include "VertexBuffer.hpp"
#include "ErrorCatcher.hpp"

VertexBuffer::~VertexBuffer() {
    GlCall(glDeleteBuffers(1, &id));
}

void VertexBuffer::init(uint32_t id, uint32_t size) {
    this->id = id;
    GlCall(glGenBuffers(1, &this->id));
    GlCall(glBindBuffer(GL_ARRAY_BUFFER, this->id));
    GlCall(glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW));
}


void VertexBuffer::bind() const {
    GlCall(glBindBuffer(GL_ARRAY_BUFFER, this->id));
}

void VertexBuffer::unBind() const {
    GlCall(glBindBuffer(GL_ARRAY_BUFFER, 0));
}

void VertexBuffer::addData(const void *data, uint32_t size, uint32_t offset) const {
    bind();
    GlCall(glBufferSubData(GL_ARRAY_BUFFER, offset, size, data));
    unBind();
}

uint32_t VertexBuffer::getOffset() {
    return offset;
}

void VertexBuffer::setOffset(int offset) {
    this->offset = offset;
}

