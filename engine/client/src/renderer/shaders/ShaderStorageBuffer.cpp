#include "ShaderStorageBuffer.hpp"

#include <GL/glew.h>

#include "core/Logger.hpp"

namespace TechEngine {
    ShaderStorageBuffer::ShaderStorageBuffer() {
    }

    ShaderStorageBuffer::~ShaderStorageBuffer() {
        glDeleteBuffers(1, &m_id);
    }

    ShaderStorageBuffer::ShaderStorageBuffer(ShaderStorageBuffer&& other) noexcept
        : m_id(other.m_id),
          m_strideBytes(other.m_strideBytes),
          m_size(other.m_size) {
        other.m_id = 0;
    }

    ShaderStorageBuffer& ShaderStorageBuffer::operator=(ShaderStorageBuffer&& other) noexcept {
        if (this != &other) {
            if (m_id != 0)
                glDeleteBuffers(1, &m_id);
            m_id = other.m_id;
            m_strideBytes = other.m_strideBytes;
            m_size = other.m_size;
            other.m_id = 0;
        }
        return *this;
    }

    void ShaderStorageBuffer::init(uint32_t size, const uint32_t strideBytes) {
        m_strideBytes = strideBytes;
        m_size = size;

        glGenBuffers(1, &m_id);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
        glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    }

    void ShaderStorageBuffer::resize(uint32_t newCapacity) {
        const uint32_t newSize = newCapacity * m_strideBytes;

        uint32_t newId = 0;
        glGenBuffers(1, &newId);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, newId);
        glBufferData(GL_SHADER_STORAGE_BUFFER, newSize, nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_COPY_READ_BUFFER, m_id);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_SHADER_STORAGE_BUFFER, 0, 0, m_size);
        glBindBuffer(GL_COPY_READ_BUFFER, 0);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        glDeleteBuffers(1, &m_id);
        m_id = newId;
        m_size = newSize;
    }

    bool ShaderStorageBuffer::isFull() const {
        return false;
    }

    void ShaderStorageBuffer::setBindingPoint(uint32_t bindingPoint) const {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, m_id);
    }

    void ShaderStorageBuffer::bind() const {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
    }

    void ShaderStorageBuffer::unBind() const {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void ShaderStorageBuffer::addData(const void* data, uint32_t size) const {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void ShaderStorageBuffer::addData(const void* data, uint32_t size, uint32_t slot) const {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, slot * m_strideBytes, size, data);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void ShaderStorageBuffer::clearData(uint32_t slot) const {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
        glClearBufferSubData(GL_SHADER_STORAGE_BUFFER, GL_R8, slot * m_strideBytes, m_strideBytes, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    uint32_t* ShaderStorageBuffer::mapBuffer(int mask) const {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
        return (uint32_t*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, mask);
    }

    void ShaderStorageBuffer::unmapBuffer() const {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void ShaderStorageBuffer::clear() const {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_id);
        glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R8, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    GLuint ShaderStorageBuffer::getID() {
        return m_id;
    }

    uint32_t ShaderStorageBuffer::getSize() const {
        return m_size;
    }

    uint32_t ShaderStorageBuffer::getStrideBytes() const {
        return m_strideBytes;
    }
}
