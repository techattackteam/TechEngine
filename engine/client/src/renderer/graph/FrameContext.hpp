#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace TechEngine {
    class SystemsRegistry;
    class GpuResourceManager;
    class SkyBox;
    struct RenderSettings;

    namespace RenderMasks {
        constexpr int SCENE = 1 << 0;
        constexpr int UI = 1 << 1;
        constexpr int LINE = 1 << 2;
        constexpr int POST_PROCESS = 1 << 3;
    }

    struct FrameContext {
        // ---- (a) per-request inputs (copied from RenderRequest) ----
        uint32_t viewId = 0;
        glm::vec3 cameraPosition = glm::vec3(0.0f);
        glm::mat4 viewMatrix = glm::mat4(1.0f);
        glm::mat4 projectionMatrix = glm::mat4(1.0f);
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
        float fov = 0.0f;
        uint32_t targetFramebufferId = 0;
        glm::vec2 viewportSize = glm::vec2(0.0f);
        int renderMask = 0;
        bool createSkyBox = false;

        // ---- (b) shared engine-lifetime handles (non-owning) ----
        SystemsRegistry* systems = nullptr;
        GpuResourceManager* gpu = nullptr;
        SkyBox* skyBox = nullptr;
        RenderSettings* settings = nullptr;

        // ---- (c) per-frame draw data produced by Renderer before the graph runs ----
        uint32_t commandToDraw = 0;

        // ---- options ----
        bool debugLightCulling = false;
        glm::uvec3 gridSize = glm::uvec3(16, 9, 24);

        // ---- (d) cross-pass CPU outputs (producer writes, later pass reads) ----
        float* currentExposure = nullptr; // storage persists on Renderer across frames
    };
}
