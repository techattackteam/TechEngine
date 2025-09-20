#pragma once

#include <cstdint>
#include "GLFW.hpp"

namespace TechEngine {
    class IndicesBuffer {
    private:
        uint32_t id;

    public:
        IndicesBuffer() = default;

        ~IndicesBuffer();

        void init(uint32_t size);

        void bind() const;

        void unBind() const;

        void addData(const void* data, uint32_t size, uint32_t offset) const;
    };
}
