#include "RenderResources.hpp"

#include <cmath>
#include <stdexcept>

#include "RenderSettings.hpp"
#include "RenderTypes.hpp"
#include "core/Logger.hpp"
#include "renderer/DrawCommand.hpp"
#include "renderer/Line.hpp"

namespace TechEngine {
    namespace {
        struct QuadVertex {
            glm::vec2 position;
            glm::vec2 texCoord;
        };
    }

    void RenderResources::init(const FroxelGridProperties& froxelGrid) {
        drawCommandBuffer = ShaderStorageBuffer();
        drawCommandBuffer.init(1000000 * sizeof(DrawCommand));
        objectDataBuffer = ShaderStorageBuffer();
        objectDataBuffer.init(1000000 * sizeof(ObjectData));

        lightsBuffer = ShaderStorageBuffer();
        lightsBuffer.init(1024 * sizeof(LightData));
        lightsIndexBuffer = ShaderStorageBuffer();
        lightsIndexBuffer.init(16 * 9 * 24 * 100 * sizeof(uint32_t));

        clusterAABBsBuffer = ShaderStorageBuffer();
        clusterAABBsBuffer.init(16 * 9 * 24 * sizeof(ClusterAABB));
        globalIndexCount = ShaderStorageBuffer();
        globalIndexCount.init(16 * 9 * 24 * sizeof(uint32_t));

        uint32_t maxWidth = 3840; // 4K
        uint32_t maxHeight = 2160;
        uint32_t maxTilesX = (maxWidth + TILE_SIZE - 1) / TILE_SIZE;
        uint32_t maxTilesY = (maxHeight + TILE_SIZE - 1) / TILE_SIZE;
        uint32_t totalMaxTiles = maxTilesX * maxTilesY;
        tileInfoBuffer = ShaderStorageBuffer();
        tileInfoBuffer.init(totalMaxTiles * sizeof(TileInfo));

        histogramBuffer = ShaderStorageBuffer();
        histogramBuffer.init(256 * sizeof(uint32_t));

        gBufferFBO = createFramebuffer(800, 600);
        FrameBuffer& depthPrePassFBO = getFramebuffer(gBufferFBO);
        depthPrePassFBO.bind();
        depthPrePassFBO.attachTexture(GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, GL_RGBA16F, GL_RGBA, GL_FLOAT, 0, 0);
        depthPrePassFBO.attachTexture(GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, GL_RGBA16F, GL_RGBA, GL_FLOAT, 0, 0);
        depthPrePassFBO.attachTexture(GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, GL_RGBA16F, GL_RGBA, GL_FLOAT, 0, 0);
        depthPrePassFBO.attachTexture(GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, GL_RGBA16F, GL_RGBA, GL_FLOAT, 0, 0);
        depthPrePassFBO.attachTexture(GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, 0, 0);
        depthPrePassFBO.unBind();

        shadowFBO = createFramebuffer(1024, 1024);
        FrameBuffer& omniShadowFBO = getFramebuffer(shadowFBO);
        omniShadowFBO.bind();
        omniShadowFBO.attachTexture(GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT, 0, 0);
        omniShadowFBO.unBind();

        QuadVertex quadVertices[4] = {
            // positions   // texCoords
            {{-1.0f, 1.0f}, {0.0f, 1.0f}},
            {{-1.0f, -1.0f}, {0.0f, 0.0f}},
            {{1.0f, 1.0f}, {1.0f, 1.0f}},
            {{1.0f, -1.0f}, {1.0f, 0.0f}},
        };
        vertexArrays[BufferFullscreenQuad] = VertexArray();
        vertexBuffers[BufferFullscreenQuad] = VertexBuffer();
        vertexArrays[BufferFullscreenQuad].init();
        vertexBuffers[BufferFullscreenQuad].init(4 * sizeof(QuadVertex));
        vertexArrays[BufferFullscreenQuad].addNewBuffer(vertexBuffers[BufferFullscreenQuad]);
        vertexBuffers[BufferFullscreenQuad].addData(quadVertices, 4 * sizeof(QuadVertex), 0);

        vertexArrays[BufferLines] = VertexArray();
        vertexBuffers[BufferLines] = VertexBuffer();
        vertexArrays[BufferLines].init();
        vertexBuffers[BufferLines].init(100000 * sizeof(Line));
        vertexArrays[BufferLines].addNewLinesBuffer(vertexBuffers[BufferLines]);

        recreateBloomTexture({800, 600});
        createNeutralLUT();
        recreateFogTexture({800, 600});
        createFroxelTexture(froxelGrid);
    }

    uint32_t RenderResources::createFramebuffer(uint32_t width, uint32_t height) {
        FrameBuffer* frameBuffer = new FrameBuffer();
        frameBuffer->init(width, height);
        m_frameBuffers.emplace_back(frameBuffer);
        return frameBuffer->getID();
    }

    FrameBuffer& RenderResources::getFramebuffer(uint32_t id) {
        for (auto& frameBuffer: m_frameBuffers) {
            if (frameBuffer->getID() == id) {
                return *frameBuffer;
            }
        }
        TE_LOGGER_CRITICAL("Framebuffer with id: %d not found", id);
        throw std::runtime_error("Framebuffer not found");
    }

    void RenderResources::createNeutralLUT(int size) {
        std::vector<glm::vec3> lutData(size * size * size);
        for (int b = 0; b < size; b++) {
            for (int g = 0; g < size; g++) {
                for (int r = 0; r < size; r++) {
                    int index = r + g * size + b * size * size;
                    lutData[index] = glm::vec3(
                        r / float(size - 1),
                        g / float(size - 1),
                        b / float(size - 1)
                    );
                }
            }
        }
        colorGradingLUT.create(GL_TEXTURE_3D, GL_RGB16F, size, size, GL_RGB, GL_FLOAT, GL_CLAMP, GL_CLAMP, lutData.data(), size);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
        glBindTexture(GL_TEXTURE_3D, 0);
    }

    void RenderResources::recreateBloomTexture(const glm::ivec2& viewport) {
        if (bloomTexture.getWidth() == viewport.x && bloomTexture.getHeight() == viewport.y) {
            return;
        }

        if (bloomTexture.getID() != 0) {
            bloomTexture.deleteTexture();
        }

        uint32_t width = viewport.x;
        uint32_t height = viewport.y;

        bloomTexture.create(GL_TEXTURE_2D, GL_RGBA16F, width, height, GL_RGBA, GL_FLOAT, GL_CLAMP, GL_CLAMP, nullptr);
        bloomIterations = floor(log2(std::min(width, height)));
        glBindTexture(GL_TEXTURE_2D, bloomTexture.getID());
        glTexStorage2D(GL_TEXTURE_2D, bloomIterations, GL_RGBA16F, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        bloomTempTexture.create(GL_TEXTURE_2D, GL_RGBA16F, width, height, GL_RGBA, GL_FLOAT, GL_CLAMP, GL_CLAMP, nullptr);
        glBindTexture(GL_TEXTURE_2D, bloomTempTexture.getID());
        glTexStorage2D(GL_TEXTURE_2D, bloomIterations, GL_RGBA16F, width, height);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);

        TE_LOGGER_INFO("Bloom texture recreated with size: {0}x{1}", width, height);
    }

    void RenderResources::recreateFogTexture(const glm::ivec2& viewport) {
        if (fogTexture.getWidth() == viewport.x && fogTexture.getHeight() == viewport.y) {
            return;
        }

        if (fogTexture.getID() != 0) {
            fogTexture.deleteTexture();
        }

        fogTexture.create(GL_TEXTURE_2D, GL_RGBA16F, viewport.x, viewport.y, GL_RGBA, GL_FLOAT, GL_CLAMP, GL_CLAMP, nullptr);
    }

    void RenderResources::createFroxelTexture(const FroxelGridProperties& froxelGrid) {
        if (froxelTexture.getID() != 0) {
            froxelTexture.deleteTexture();
            volumetricLightVolume.deleteTexture();
        }

        froxelTexture.create(GL_TEXTURE_3D, GL_RGBA16F, froxelGrid.width, froxelGrid.height, GL_RGBA, GL_FLOAT, GL_CLAMP, GL_CLAMP, nullptr, froxelGrid.depth);
        volumetricLightVolume.create(GL_TEXTURE_3D, GL_RGBA16F, froxelGrid.width, froxelGrid.height, GL_RGBA, GL_FLOAT, GL_CLAMP, GL_CLAMP, nullptr, froxelGrid.depth);

        glGenBuffers(1, &froxelParamsUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, froxelParamsUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(FroxelParams), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        glGenBuffers(1, &volumetricSettingsUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, volumetricSettingsUBO);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(VolumetricSettings), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
}
