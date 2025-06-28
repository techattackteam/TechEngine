#include "UIView.hpp"

#include "renderer/FrameBuffer.hpp"
#include "renderer/Renderer.hpp"
#include "systems/SystemsRegistry.hpp"


namespace TechEngine {
    UIView::UIView(SystemsRegistry& editorSystemsRegistry,
                   SystemsRegistry& appSystemsRegistry) : Panel(editorSystemsRegistry),
                                                          m_appSystemsRegistry(appSystemsRegistry) {
        m_styleVars.emplace_back(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    }

    void UIView::onInit() {
        m_frameBufferID = m_appSystemsRegistry.getSystem<Renderer>().createFramebuffer(1080, 720);
        m_context = m_appSystemsRegistry.getSystem<Renderer>().getUIContext();
    }

    void UIView::onUpdate() {
        ImVec2 rmlViewSize = ImGui::GetContentRegionAvail();
        FrameBuffer& frameBuffer = m_appSystemsRegistry.getSystem<Renderer>().getFramebuffer(m_frameBufferID);
        if (rmlViewSize.x <= 0) rmlViewSize.x = 1;
        if (rmlViewSize.y <= 0) rmlViewSize.y = 1;
        // Resize FBO if RML View size changed
        if (m_frameBufferID && (static_cast<int>(rmlViewSize.x) != frameBuffer.width || static_cast<int>(rmlViewSize.y) != frameBuffer.height)) {
            if (rmlViewSize.x > 0 && rmlViewSize.y > 0) {
                frameBuffer.resize(rmlViewSize.x, rmlViewSize.y);
                m_context->SetDimensions(Rml::Vector2i(frameBuffer.width, frameBuffer.height));
            }
        }
        UIRenderer& uiRenderer = m_appSystemsRegistry.getSystem<Renderer>().getUIRenderer();
        uiRenderer.onUpdate();
        ImGui::Image((void*)(intptr_t)frameBuffer.colorTexture, ImVec2((float)frameBuffer.width, (float)frameBuffer.height), ImVec2(0, 1), ImVec2(1, 0));
    }
}
