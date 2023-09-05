#include "GameView.hpp"
#include "renderer/RendererSettings.hpp"

namespace TechEngine {

    GameView::GameView(Renderer &renderer) : renderer(&renderer), Panel("Game") {
        frameBufferID = renderer.createFramebuffer(RendererSettings::width, RendererSettings::height);
    }

    void GameView::onUpdate() {
        FrameBuffer &frameBuffer = renderer->getFramebuffer(frameBufferID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        ImGui::Begin(name.c_str());
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        frameBuffer.bind();
        frameBuffer.resize(wsize.x, wsize.y);
        renderer->renderPipeline();
        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void *>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        frameBuffer.unBind();
        ImGui::End();
        ImGui::PopStyleVar();
    }
}
