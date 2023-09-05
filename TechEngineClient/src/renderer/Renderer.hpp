#pragma once

#include "components/MeshRendererComponent.hpp"
#include "VertexArray.hpp"
#include "ShadersManager.hpp"
#include "FrameBuffer.hpp"
#include "scene/Scene.hpp"

namespace TechEngine {
    class Renderer {
    private:
        uint32_t id = 1;

        ShadersManager shadersManager;
        VertexArray vertexArray;
        VertexBuffer vertexBuffer;
        std::list<FrameBuffer *> frameBuffers;
        Scene &scene = Scene::getInstance();
    public:

        Renderer() = default;
        ~Renderer();
        void init();

        void renderPipeline();

        void flushMeshData(MeshRendererComponent *meshRenderer);

        FrameBuffer &getFramebuffer(uint32_t id);

        uint32_t createFramebuffer(uint32_t width, uint32_t height);

    private:
        void shadowPass();

        void geometryPass();

        void renderWithLightPass();

        void renderGeometryPass(bool shadow);

        void renderGameObject(GameObject *gameObject, bool shadow);
    };
}

