#pragma once

#include "VertexArray.hpp"
#include "ShadersManager.hpp"

namespace Engine {
    class Renderer {
    private:
        uint32_t id = 1;
        float width, height;

        VertexArray vertexArray;
        VertexBuffer vertexBuffer;
        ShadersManager shadersManager;
    public:
        Renderer() = default;

        void init(float width, float height);

        void ImGuiPipeline() const;

        void renderPipeline();

        static void beginImGuiFrame();
    };
}

