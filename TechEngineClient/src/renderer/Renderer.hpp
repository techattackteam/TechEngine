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
        FrameBuffer shadowMapBuffer;
        FrameBuffer frameBuffer;
        Scene &scene = Scene::getInstance();
    public:

        Renderer() = default;

        void init();

        void renderPipeline();

        void flushMeshData(MeshRendererComponent *meshRenderer);

        FrameBuffer &getFramebuffer();

    private:
        void shadowPass();

        void geometryPass();

        void renderWithLightPass();

        void renderGeometryPass(bool shadow);

        void renderGameObject(GameObject *gameObject, bool shadow);
    };
}

