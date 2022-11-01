#include "RendererPanel.hpp"
#include "renderer/RendererSettings.hpp"

namespace TechEngine {
    RendererPanel::RendererPanel(Renderer &renderer) : renderer(&renderer), Panel("Scene") {
        renderer.init();
        RendererSettings::changeTarget(renderer.getFramebuffer().getID());
    }


    void RendererPanel::onUpdate() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
        ImGui::Begin(name.c_str());
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        renderer->getFramebuffer().resize(wsize.x, wsize.y);
        renderer->getFramebuffer().bind();
        renderer->renderPipeline();
        uint64_t textureID = renderer->getFramebuffer().getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void *>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        renderer->getFramebuffer().unBind();
        ImGui::End();
        ImGui::PopStyleVar();
    }
}
