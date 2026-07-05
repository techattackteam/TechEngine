#pragma once
#include <cstdint>
#include <stack>
#include <GL/glew.h>

namespace TechEngine {
    class ShaderStorageBuffer {
    private:
        uint32_t m_id = 0;
        uint32_t m_strideBytes = 0;
        uint32_t m_size = 0;

    public:
        ShaderStorageBuffer();

        ~ShaderStorageBuffer();

        ShaderStorageBuffer(const ShaderStorageBuffer&) = delete;

        ShaderStorageBuffer& operator=(const ShaderStorageBuffer&) = delete;

        ShaderStorageBuffer(ShaderStorageBuffer&& other) noexcept;

        ShaderStorageBuffer& operator=(ShaderStorageBuffer&& other) noexcept;

        void init(uint32_t size, uint32_t strideBytes = 0);

        void resize(uint32_t newCapacity);

        bool isFull() const;

        void setBindingPoint(uint32_t bindingPoint) const;

        void bind() const;

        void unBind() const;

        void addData(const void* data, uint32_t size) const;

        void addData(const void* data, uint32_t size, uint32_t slot) const;

        void clearData(uint32_t slot) const;

        uint32_t* mapBuffer(int mask) const;

        void unmapBuffer() const;

        void clear() const;

        GLuint getID();

        uint32_t getSize() const;

        uint32_t getStrideBytes() const;
    };
}
