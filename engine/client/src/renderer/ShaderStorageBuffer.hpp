#pragma once
#include <cstdint>
#include <GL/glew.h>

namespace TechEngine {
    class ShaderStorageBuffer {
    private:
        uint32_t id = 0;

    public:
        ShaderStorageBuffer();

        ~ShaderStorageBuffer();

        void init(uint32_t size);

        void setBindingPoint(uint32_t bindingPoint) const;

        void bind();

        void unBind() const;

        void addData(const void* data, uint32_t size) const;

        void addData(const void* data, uint32_t size, uint32_t offset) const;

        GLuint getID();
    };
}
