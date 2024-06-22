#include "GameView.hpp"
#include "imgui.h"
#include "scene/SceneManager.hpp"

namespace TechEngine {
    GameView::GameView(SystemsRegistry& appRegistry) : appRegistry(appRegistry), Panel("Game") {
    }

    void GameView::init() {
        frameBufferID = appRegistry.getSystem<Renderer>().createFramebuffer(1080, 720);
    }

    void GameView::onUpdate() {
        Renderer& renderer = appRegistry.getSystem<Renderer>();
        if (!appRegistry.getSystem<SceneManager>().getScene().hasMainCamera()) {
            return;
        }
        FrameBuffer& frameBuffer = renderer.getFramebuffer(frameBufferID);
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        frameBuffer.bind();
        frameBuffer.resize(wsize.x, wsize.y);
        appRegistry.getSystem<SceneManager>().getScene().getMainCamera()->updateProjectionMatrix(wsize.x / wsize.y);
        renderer.renderPipeline(appRegistry.getSystem<SceneManager>().getScene());
        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void*>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        frameBuffer.unBind();
    }
}
