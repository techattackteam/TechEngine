#include <vector>
#include <glm/glm.hpp>

#include "GLFW.hpp"

#pragma once
namespace TechEngine {
    class VertexBuffer {
    private:
        uint32_t id;
        uint32_t offset = 0;

    public:
        VertexBuffer() = default;

        ~VertexBuffer();

        void init(uint32_t size);

        void bind() const;

        void unBind() const;

        void addData(const void *data, uint32_t size, uint32_t offset) const;

        uint32_t getOffset();

        void setOffset(int offset);
    };
}