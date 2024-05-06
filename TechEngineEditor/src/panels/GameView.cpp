#include "GameView.hpp"
#include "imgui.h"

namespace TechEngine {
    GameView::GameView(EventDispatcher& eventDispatcher, Renderer& renderer, Scene& scene) : renderer(&renderer), scene(scene), Panel("Game", eventDispatcher) {
        frameBufferID = renderer.createFramebuffer(1080, 720);
    }

    void GameView::onUpdate() {
        if (!scene.hasMainCamera()) {
            return;
        }
        FrameBuffer& frameBuffer = renderer->getFramebuffer(frameBufferID);
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        frameBuffer.bind();
        frameBuffer.resize(wsize.x, wsize.y);
        scene.getMainCamera()->updateProjectionMatrix(wsize.x / wsize.y);
        renderer->renderPipeline(scene);
        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void*>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        frameBuffer.unBind();
    }
}
