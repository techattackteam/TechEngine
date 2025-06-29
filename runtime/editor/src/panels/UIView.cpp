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
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        FrameBuffer& frameBuffer = m_appSystemsRegistry.getSystem<Renderer>().getFramebuffer(m_frameBufferID);
        frameBuffer.bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        frameBuffer.resize(wsize.x, wsize.y);
        m_context->SetDimensions(Rml::Vector2i(frameBuffer.width, frameBuffer.height));
        m_context->Update();
        UIRenderer& uiRenderer = m_appSystemsRegistry.getSystem<Renderer>().getUIRenderer();
        uiRenderer.onUpdate();
        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void*>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        frameBuffer.unBind();
    }
}
