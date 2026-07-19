#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <glm/glm.hpp>

#include "renderer/FrameBuffer.hpp"
#include "renderer/mesh/IndicesBuffer.hpp"
#include "renderer/mesh/VertexArray.hpp"
#include "renderer/mesh/VertexBuffer.hpp"
#include "renderer/resources/Texture.hpp"
#include "renderer/shaders/ShaderStorageBuffer.hpp"

namespace TechEngine {
    struct FroxelGridProperties;

    class RenderResources {
    public:
        static constexpr int32_t TILE_SIZE = 16;

        inline static const std::string BufferLines = "Lines";
        inline static const std::string BufferFullscreenQuad = "FullscreenQuad";

        std::vector<FrameBuffer*> m_frameBuffers;
        uint32_t gBufferFBO = 0; // depth - depth map, 0 - albedo, 1 - normals, 2 - screenLight, 3 - hdr color
        uint32_t shadowFBO = 0;

        // Geometry / instancing buffers
        ShaderStorageBuffer drawCommandBuffer;
        ShaderStorageBuffer objectDataBuffer;

        // Lighting buffers
        ShaderStorageBuffer lightsBuffer;
        ShaderStorageBuffer lightsIndexBuffer;
        ShaderStorageBuffer tileInfoBuffer;
        ShaderStorageBuffer clusterAABBsBuffer;
        ShaderStorageBuffer globalIndexCount;

        // Auto-exposure histogram (256 bins)
        ShaderStorageBuffer histogramBuffer;

        std::unordered_map<std::string, VertexArray> vertexArrays;
        std::unordered_map<std::string, VertexBuffer> vertexBuffers;
        std::unordered_map<std::string, IndicesBuffer> indicesBuffers;

        // Post-process / effect targets
        Texture aoTexture;
        Texture bloomTexture;
        Texture bloomTempTexture;
        int bloomIterations = 0;
        Texture colorGradingLUT;
        Texture fogTexture;
        Texture froxelTexture;
        Texture volumetricLightVolume;
        uint32_t froxelParamsUBO = 0;
        uint32_t volumetricSettingsUBO = 0;

        RenderResources() = default;

        // Creates every persistent GPU resource. Called once from Renderer::init().
        void init(const FroxelGridProperties& froxelGrid);

        uint32_t createFramebuffer(uint32_t width, uint32_t height);

        FrameBuffer& getFramebuffer(uint32_t id);

        void recreateBloomTexture(const glm::ivec2& viewport);

        void recreateFogTexture(const glm::ivec2& viewport);

        void createFroxelTexture(const FroxelGridProperties& froxelGrid);

        void createNeutralLUT(int size = 32);
    };
}
