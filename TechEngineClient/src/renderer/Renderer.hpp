#pragma once

#include "components/MeshRendererComponent.hpp"
#include "VertexArray.hpp"
#include "ShadersManager.hpp"
#include "FrameBuffer.hpp"
#include "scene/Scene.hpp"
#include "Line.hpp"
#include "IndicesBuffer.hpp"

namespace TechEngine {
    class Renderer {
    private:
        const std::string BufferGameObjects = "GameObjects";
        const std::string BufferLines = "Lines";
        uint32_t id = 0;

        std::vector<Line> lines;

        ShadersManager shadersManager;
        std::unordered_map<std::string, VertexArray*> vertexArrays;
        std::unordered_map<std::string, VertexBuffer*> vertexBuffers;
        std::unordered_map<std::string, IndicesBuffer*> indicesBuffers;
        std::list<FrameBuffer*> frameBuffers;

    public:
        Renderer() = default;

        ~Renderer();

        void init(ProjectManager& projectManager);

        void renderPipeline(Scene& scene);

        void renderCustomPipeline(std::vector<GameObject*>& gameObjects);

        void createLine(const glm::vec3& startPosition, const glm::vec3& endPosition, const glm::vec4& color);

        FrameBuffer& getFramebuffer(uint32_t id);

        uint32_t createFramebuffer(uint32_t width, uint32_t height);

        ShadersManager& getShadersManager();

    private:
        void shadowPass(Scene& scene);

        void geometryPass(Scene& scene);

        void linePass();

        void renderWithLightPass(Scene& scene);

        void renderGeometryPass(Scene& scene, bool shadow);

        void renderGameObject(GameObject* gameObject, bool shadow);

        void flushMeshData(MeshRendererComponent* meshRenderer);

        void flushLinesData();
    };
}
