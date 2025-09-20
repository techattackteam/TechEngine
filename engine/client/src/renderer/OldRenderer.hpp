#pragma once

#include "core/ExportDll.hpp"
#include "systems/System.hpp"
#include "ShadersManager.hpp"
#include "Line.hpp"
#include "ShaderStorageBuffer.hpp"
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

    class CLIENT_DLL OldRenderer : public System {
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

        ShaderStorageBuffer drawCommandBuffer;
        ShaderStorageBuffer objectDataBuffer;
        ShaderStorageBuffer materialsBuffer;

        std::list<FrameBuffer*> frameBuffers;
        UIRenderer uiRenderer;

        uint32_t currentVertexOffset = 0; // Tracks the end of the VBO data (in vertices)
        uint32_t currentIndexOffset = 0; // Tracks the end of the IBO data (in indices)
        size_t commandToDraw;

    public:
        OldRenderer(SystemsRegistry& systemsRegistry);

        ~OldRenderer();

        void init();

        void shutdown() override;

        void renderPipeline(Camera& camera);

        void renderCustomPipeline(Camera* camera, std::vector<int>& entities);

        void uiPass();
#pragma region NEW RENDERER
        void renderIndirectInstanced(Camera& camera);

        void renderIndirectMultiMesh(Camera& camera);

        void prepareMDIRender();

        void loadMesh(Mesh& mesh);

        void loadMaterials();
#pragma endregion
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
