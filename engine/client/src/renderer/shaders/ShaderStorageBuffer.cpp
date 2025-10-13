#include "ShaderStorageBuffer.hpp"

#include <GL/glew.h>

namespace TechEngine {
    ShaderStorageBuffer::ShaderStorageBuffer() {
    }

    ShaderStorageBuffer::~ShaderStorageBuffer() {
        glDeleteBuffers(1, &id);
    }

    void ShaderStorageBuffer::init(uint32_t size) {
        glGenBuffers(1, &id);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    }

    void ShaderStorageBuffer::setBindingPoint(uint32_t bindingPoint) const {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, id);
    }

    void ShaderStorageBuffer::bind() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
    }

    void ShaderStorageBuffer::unBind() const {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void ShaderStorageBuffer::addData(const void* data, uint32_t size) const {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void ShaderStorageBuffer::addData(const void* data, uint32_t size, uint32_t offset) const {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, data);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    uint32_t* ShaderStorageBuffer::mapBuffer(int mask) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
        return (uint32_t*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, mask);
    }

    void ShaderStorageBuffer::unmapBuffer() {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void ShaderStorageBuffer::clear() const {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    GLuint ShaderStorageBuffer::getID() {
        return id;
    }
}
