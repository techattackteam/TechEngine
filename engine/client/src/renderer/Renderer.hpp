#pragma once

#include "core/ExportDLL.hpp"
#include "systems/System.hpp"

#include "ShadersManager.hpp"
#include "ShaderStorageBuffer.hpp"
#include "VertexArray.hpp"
#include "VertexBuffer.hpp"
#include "IndicesBuffer.hpp"
#include "FrameBuffer.hpp"
#include "Line.hpp"
#include "RenderRequest.hpp"
#include "components/Components.hpp"
#include "scene/Scene.hpp"
#include "ui/UIRenderer.hpp"

#include <queue>

namespace TechEngine {
    class CLIENT_DLL Renderer : public System {
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

        std::list<FrameBuffer> frameBuffers;
        UIRenderer m_uiRenderer;

        uint32_t m_currentVertexOffset = 0; // Tracks the end of the VBO data (in vertices)
        uint32_t m_currentIndexOffset = 0; // Tracks the end of the IBO data (in indices)
        size_t m_commandToDraw = 0;

        std::vector<Line> lines;

    public:
        inline static const int GEOMETRY_PASS = 1 << 0;
        inline static const int UI_PASS = 1 << 1;
        inline static const int LINE_PASS = 1 << 2;

        Renderer(SystemsRegistry& systemsRegistry);

        ~Renderer() override;

        void init() override;

        void onStart() override;

        void onUpdate() override;

        void shutdown() override;

        void addRequest(const RenderRequest& request);

        void renderPipeline();

        void renderCustomPipeline(Camera& camera, int mask);

        void createLine(const glm::vec3& startPosition, const glm::vec3& endPosition, const glm::vec4& color);

        uint32_t createFramebuffer(uint32_t width, uint32_t height);

        FrameBuffer& getFramebuffer(uint32_t id);

        UIRenderer& getUIRenderer();

    private:
        void uploadNewMesh(const std::string& name);

        void removeMesh(Mesh& mesh);

        void uploadNewMaterial(const std::string& name);

        void removeMaterial(const std::string& name);

        void populateDataBuffers(Scene& scene);

        void geometryPass(glm::mat4 viewMatrix, glm::mat4 projectionMatrix);

        void uiPass();

        void linePass(glm::mat4 viewMatrix, glm::mat4 projectionMatrix);
    };
}
