#pragma once

#include "TechEngine/client/core/ExportDLL.hpp"
#include "systems/System.hpp"

#include "ShadersManager.hpp"
#include "ShaderStorageBuffer.hpp"
#include "VertexArray.hpp"
#include "VertexBuffer.hpp"
#include "IndicesBuffer.hpp"
#include "FrameBuffer.hpp"
#include "Line.hpp"
#include "RenderRequest.hpp"
#include "TechEngine/core/components/Components.hpp"
#include "TechEngine/core/scene/Scene.hpp"
#include "ui/UIRenderer.hpp"

#include <queue>

namespace TechEngine {
    class CLIENT_DLL Renderer : public System {
        // Temp before creating the light component
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

    private:
        const std::string BufferGameObjects = "GameObjects";
        const std::string BufferLines = "Lines";

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

        const int32_t TILE_SIZE = 16;

        uint32_t m_depthPrePassFBO = 0;
        uint32_t m_omniShadowFBO = 0;

        std::vector<FrameBuffer*> m_frameBuffers;

        UIRenderer m_uiRenderer;

        uint32_t m_currentVertexOffset = 0; // Tracks the end of the VBO data (in vertices)
        uint32_t m_currentIndexOffset = 0; // Tracks the end of the IBO data (in indices)
        size_t m_commandToDraw = 0;

        std::vector<Renderable> m_renderables;

        std::vector<Line> lines;

    public:
        inline static const int SCENE_PASS = 1 << 0;
        inline static const int UI_PASS = 1 << 1;
        inline static const int LINE_PASS = 1 << 2;

        Renderer(SystemsRegistry& systemsRegistry);

        ~Renderer() override;

        // 2. Disable Copying
        // This will prevent anyone from accidentally copying the renderer
        Renderer(const Renderer&) = delete;

        Renderer& operator=(const Renderer&) = delete;

        // 3. Enable Moving (Good Practice)
        // This allows you to, for example, return a Renderer from a factory function
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

        void populateLightDataBuffers();

        void populateObjectDataBuffers() const;

        void scenePass(const RenderRequest& request);

        void depthPrePass(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport);

        void omniShadowPass(float nearPlane, float farPlane, const glm::ivec2& viewport);

        void lightCulling(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport);

        void geometryPass(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const glm::ivec2& viewport, float farPlane);

        void uiPass();

        void linePass(glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
    };
}
