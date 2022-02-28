#pragma once

#include "../components/MeshRendererComponent.hpp"
#include "VertexArray.hpp"
#include "ShadersManager.hpp"
#include "FrameBuffer.hpp"
#include "../components/DirectionalLightComponent.hpp"

namespace Engine {
    class Renderer {
    private:
        uint32_t id = 1;
        int width, height;

        ShadersManager shadersManager;
        VertexArray vertexArray;
        VertexBuffer vertexBuffer;
        FrameBuffer shadowMapBuffer;

        //TEMP FOR JUST ONE LIGHT
        DirectionalLightComponent *light;
    public:

        Renderer() = default;

        void init(int width, int height);

        void ImGuiPipeline() const;

        void renderPipeline();

        void flushMeshData(MeshRendererComponent *meshComponent);

        static void beginImGuiFrame();

    private:
        void lightPass();

        void geometryPass();

        void renderPass();


        void renderQuad();
    };
}

