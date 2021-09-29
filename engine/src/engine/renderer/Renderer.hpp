#pragma once

#include "../components/MeshRendererComponent.hpp"
#include "VertexArray.hpp"
#include "ShadersManager.hpp"

namespace Engine {
    class Renderer {
    private:
        uint32_t id = 1;
        float width, height;

    public:
        Renderer() = default;

        void init(float width, float height);

        void ImGuiPipeline() const;

        void renderPipeline();

        void flushMeshData(MeshRendererComponent *meshComponent);

        static void beginImGuiFrame();

        ShadersManager shadersManager;
        VertexArray vertexArray;
        VertexBuffer vertexBuffer;
    };
}

