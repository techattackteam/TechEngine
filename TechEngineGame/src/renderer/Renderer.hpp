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

    public:

        Renderer() = default;

        void init();

        void ImGuiPipeline() const;

        void renderPipeline();

        void flushMeshData(MeshRendererComponent *meshRenderer);

        static void beginImGuiFrame();

    private:
        void shadowPass();

        void geometryPass();

        void renderWithLightPass();

        void renderGeometryPass(bool shadow);
    };
}

