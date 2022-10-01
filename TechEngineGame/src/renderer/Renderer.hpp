#pragma once

#include "../components/MeshRendererComponent.hpp"
#include "VertexArray.hpp"
#include "ShadersManager.hpp"
#include "FrameBuffer.hpp"
#include "../components/DirectionalLightComponent.hpp"

namespace TechEngine {
    class Renderer {
    private:
        uint32_t id = 1;

        ShadersManager shadersManager;
        VertexArray vertexArray;
        VertexBuffer vertexBuffer;
        FrameBuffer shadowMapBuffer;
        FrameBuffer frameBuffer;
    public:

        Renderer() = default;

        void init(uint32_t width, uint32_t height);

        void renderPipeline();

        void flushMeshData(MeshRendererComponent *meshRenderer);

        FrameBuffer &getFramebuffer();

    private:
        void shadowPass();

        void geometryPass();

        void renderWithLightPass();

        void renderGeometryPass(bool shadow);
    };
}

