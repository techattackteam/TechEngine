#pragma once
#include <glm/glm.hpp>

namespace TechEngine {
    struct RenderRequest {
        uint32_t viewId;

        // The camera to use for this view.
        // Note: We copy the camera state, not a pointer. This is thread-safe
        // and prevents the camera from changing under the renderer's feet.
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 projectionMatrix = glm::mat4(1.0f);

        // The target framebuffer to render into.
        uint32_t targetFramebufferId;

        // The dimensions of the target viewport.
        glm::vec2 viewportSize;

        // A bitmask to control which passes to run (from your Renderer.hpp).
        int renderMask;
    };
}
