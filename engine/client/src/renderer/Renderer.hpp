#pragma once

#include "core/ExportDll.hpp"
#include "systems/System.hpp"
#include "ShadersManager.hpp"
#include "Line.hpp"
#include "scene/CameraSystem.hpp"
#include "ui/UIRenderer.hpp"

namespace TechEngine {
    class Scene;
    class FrameBuffer;
    class IndicesBuffer;
    class VertexBuffer;
    class VertexArray;
    class Camera;
    class MeshRenderer;

    class CLIENT_DLL Renderer : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        const std::string BufferGameObjects = "GameObjects";
        const std::string BufferLines = "Lines";
        uint32_t id = 0;

        std::vector<Line> lines;

        ShadersManager shadersManager;
        std::unordered_map<std::string, VertexArray*> vertexArrays;
        std::unordered_map<std::string, VertexBuffer*> vertexBuffers;
        std::unordered_map<std::string, IndicesBuffer*> indicesBuffers;
        std::list<FrameBuffer*> frameBuffers;
        UIRenderer uiRenderer;

    public:
        Renderer(SystemsRegistry& systemsRegistry);

        ~Renderer();

        void init();

        void shutdown() override;

        void renderPipeline(Camera& camera);

        void renderCustomPipeline(Camera* camera, std::vector<int>& entities);

        void uiPass();

        void createLine(const glm::vec3& startPosition, const glm::vec3& endPosition, const glm::vec4& color);

        FrameBuffer& getFramebuffer(uint32_t id);

        uint32_t createFramebuffer(uint32_t width, uint32_t height);

        ShadersManager& getShadersManager();

        UIRenderer& getUIRenderer();

    private:
        void shadowPass(Scene& scene);

        void geometryPass(Camera& camera);


        void linePass(Camera& camera);

        void renderWithLightPass(Scene& scene);

        void renderGeometryPass(Scene& scene, bool shadow);

        void renderMesh(Transform& transform, MeshRenderer& meshRenderer, bool shadow);

        void flushMeshData(MeshRenderer* meshRenderer);

        void flushLinesData();
    };
}
