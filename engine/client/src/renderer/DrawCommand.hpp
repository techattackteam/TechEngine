#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace TechEngine {
    struct DrawCommand {
        uint32_t count;
        uint32_t instanceCount;
        uint32_t firstIndex;
        uint32_t baseVertex;
        uint32_t baseInstance;
    };

    struct ObjectData {
        glm::mat4 modelMatrix; // 64 bytes
        uint32_t materialIndex; // 4 bytes
        uint32_t padding[3]; // Padding to make the size a multiple of 16 bytes
    };
}
