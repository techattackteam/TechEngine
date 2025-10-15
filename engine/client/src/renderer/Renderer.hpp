#pragma once

#include "TechEngine/client/core/ExportDLL.hpp"
#include "systems/System.hpp"

#include "shaders/ShadersManager.hpp"
#include "shaders/ShaderStorageBuffer.hpp"
#include "mesh/VertexArray.hpp"
#include "mesh/VertexBuffer.hpp"
#include "mesh/IndicesBuffer.hpp"
#include "FrameBuffer.hpp"
#include "Line.hpp"
#include "RenderRequest.hpp"
#include "TechEngine/core/components/Components.hpp"
#include "TechEngine/core/scene/Scene.hpp"
#include "ui/UIRenderer.hpp"

#include <queue>

#include "texture/SkyBox.hpp"
#include "texture/Texture.hpp"

namespace TechEngine {
    class CLIENT_DLL Renderer : public System {
        // Temp before creating the light component
        struct Light {
            glm::vec3 position; // 12 bytes
            float padding1; // 4 byte to align to 16 bytes match std430 layout
            glm::vec3 color; // 12 bytes
            float radius; // 4 byte
            float intensity; // 4 byte
            float padding3[3]; // Padding to align to 16 bytes
        };

        struct TileInfo {
            uint32_t offset;
            uint32_t lightCount;
        };

        struct Renderable {
            Transform* transform;
            MeshRenderer* meshRenderer;

            // A comparison operator for std::sort
            bool operator<(const Renderable& other) const {
                // Sort primarily by mesh, then by material, for maximum batching.
                if (meshRenderer->mesh < other.meshRenderer->mesh) return true;
                if (meshRenderer->mesh > other.meshRenderer->mesh) return false;
                // If meshes are the same, you could optionally sort by material as a secondary key.
                return meshRenderer->material->getGpuID() < other.meshRenderer->material->getGpuID();
            }
        };

        struct QuadVertex {
            glm::vec2 position;
            glm::vec2 texCoord;
        };

    private:
        const std::string BufferGameObjects = "GameObjects";
        const std::string BufferLines = "Lines";
        const std::string BufferFullscreenQuad = "FullscreenQuad";

        SystemsRegistry& m_systemsRegistry;

        std::queue<RenderRequest> m_renderQueue;

        ShadersManager m_shadersManager;
        std::unordered_map<std::string, VertexArray> m_vertexArrays;
        std::unordered_map<std::string, VertexBuffer> m_vertexBuffers;
        std::unordered_map<std::string, IndicesBuffer> m_indicesBuffers;

        ShaderStorageBuffer m_drawCommandBuffer;
        ShaderStorageBuffer m_objectDataBuffer;
        ShaderStorageBuffer m_materialsBuffer;
        ShaderStorageBuffer m_lightsBuffer;

        // Buffers for light culling
        ShaderStorageBuffer m_lightsIndexBuffer;
        ShaderStorageBuffer m_tileInfoBuffer;
        ShaderStorageBuffer m_atomicCounterBuffer;

        // 256 bins for the histogram
        ShaderStorageBuffer m_histogramBuffer;

        const int32_t TILE_SIZE = 16;

        uint32_t m_depthPrePassFBO = 0;
        uint32_t m_shadowFBO = 0;
        uint32_t m_hdrFBO = 0;
        uint32_t m_gtaoFBO = 0;
        uint32_t m_gtaoPingPongFBO = 0;

        std::vector<FrameBuffer*> m_frameBuffers;

        UIRenderer m_uiRenderer;

        uint32_t m_currentVertexOffset = 0; // Tracks the end of the VBO data (in vertices)
        uint32_t m_currentIndexOffset = 0; // Tracks the end of the IBO data (in indices)
        size_t m_commandToDraw = 0;

        float m_currentExposure = 1.0f;
        float m_targetExposure = 1.0f;
        float m_adaptationSpeed = 1.5f;

        std::vector<Renderable> m_renderables;

        std::vector<Texture> m_materialsTextures;

        Texture m_gtaoNoiseTexture;
        Texture m_depthNormalTexture;
        Texture m_bentNormal;
        Texture m_motionVectorTexture;
        Texture m_aoHalfTextures[2];
        uint32_t m_aoHalfTextureIndex = 0;
        Texture m_aoFullTexture;

        Texture m_bloomTexture;
        Texture m_bloomTempTexture;
        int m_bloomIterations = 0;

        std::vector<Line> lines;

        SkyBox m_skyBox;

    public:
        inline static const int SCENE_PASS = 1 << 0;
        inline static const int UI_PASS = 1 << 1;
        inline static const int LINE_PASS = 1 << 2;
        inline static const int POST_PROCESS_PASS = 1 << 3;

        Renderer(SystemsRegistry& systemsRegistry);

        ~Renderer() override;

        Renderer(const Renderer&) = delete;

        Renderer& operator=(const Renderer&) = delete;

        Renderer(Renderer&&) noexcept = default;

        Renderer& operator=(Renderer&&) noexcept = delete;

        void init() override;

        void onStart() override;

        void onUpdate() override;

        void shutdown() override;

        void addRequest(const RenderRequest& request);

        void renderPipeline();

        void createLine(const glm::vec3& startPosition, const glm::vec3& endPosition, const glm::vec4& color);

        uint32_t createFramebuffer(uint32_t width, uint32_t height);

        FrameBuffer& getFramebuffer(uint32_t id);

        UIRenderer& getUIRenderer();

    private:
        void uploadNewMesh(const std::string& name);

        void removeMesh(Mesh& mesh);

        void uploadNewMaterial(const std::string& name);

        void removeMaterial(const std::string& name);

        void createRenderables();

        void populateLightDataBuffers() const;

        void populateObjectDataBuffers() const;

        void populateMaterialDataBuffers();

        void recreateBloomTexture(const glm::ivec2& viewport);

        void scenePass(const RenderRequest& request);

        void depthPrePass(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport);

        void gtaoPass(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport);

        void lightCulling(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport);

        void shadowDepthPass(const RenderRequest& request);

        void geometryPass(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport, float farPlane);

        void automaticExposurePass(const glm::ivec2& viewport);

        void bloomPass(const glm::ivec2& viewport);

        void postProcessingPass();

        void uiPass();

        void linePass(glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
    };
}
