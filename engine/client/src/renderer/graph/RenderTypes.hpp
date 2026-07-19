#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace TechEngine {
    class SystemsRegistry;
    class ShaderStorageBuffer;

    struct alignas(16) LightData {
        // -- Block 1: 16 bytes --
        glm::vec3 position = glm::vec3(0.0f);
        int type = 0;

        // -- Block 2: 16 bytes --
        glm::vec3 direction = glm::vec3(0.0f);
        float radius = 0.0f;

        // -- Block 3: 16 bytes --
        glm::vec3 color;
        float intensity = 0.0f;

        // -- Block 4: 16 bytes --
        float innerCutoff = 0.0f;
        float outerCutoff = 0.0f;
        int castShadow = 0;
        float _pad2;

        // -- Block 5 & 6: 32 bytes --
        uint64_t shadowTextureHandle[4] = {0}; // 8 byte

        // -- Block 7 - 10: 64 bytes * 4 = 256 bytes --
        glm::mat4 lightSpaceMatrix[4] = {glm::mat4(1.0f)}; // 64 bytes

        // -- Block 11: 16 bytes --
        float cascadeSplits[4] = {0.0f};
    };

    struct TileInfo {
        uint32_t offset;
        uint32_t lightCount;
    };

    struct ClusterAABB {
        glm::vec4 minPoint;
        glm::vec4 maxPoint;
    };

    // Gathers all lights from the active scene and uploads them into the given lights SSBO.
    // Shared between Renderer (per-frame update) and ShadowDepthPass (re-upload after shadow handles change).
    void populateLightsBuffer(SystemsRegistry& systemsRegistry, const ShaderStorageBuffer& lightsBuffer);
}
