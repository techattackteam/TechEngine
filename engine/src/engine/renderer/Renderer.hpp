#pragma once
namespace Engine {
    class Renderer {
    private:
        float width, height;

    public:
        Renderer(float width, float height);

        void pollEvents();

        void ImGuiPipeline();

        void renderPipeline();

        void beginImGuiFrame();
    };
}

