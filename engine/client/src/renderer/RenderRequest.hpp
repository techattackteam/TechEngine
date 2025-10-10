#pragma once
#include <glm/glm.hpp>

namespace TechEngine {
    struct RenderRequest {
        uint32_t viewId;

        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 projectionMatrix = glm::mat4(1.0f);

        float nearPlane = 0.1f;
        float farPlane = 1000.0f;

        uint32_t targetFramebufferId;

        glm::vec2 viewportSize;
        float fov;

        int renderMask;
    };
}
