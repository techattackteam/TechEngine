#include "UIView.hpp"

#include <imGuizmo.h>

#include "UIEditor.hpp"
#include "renderer/FrameBuffer.hpp"
#include "renderer/Renderer.hpp"
#include "../ui/Widget.hpp"
#include "systems/SystemsRegistry.hpp"


namespace TechEngine {
    UIView::UIView(SystemsRegistry& editorSystemsRegistry,
                   SystemsRegistry& appSystemsRegistry,
                   UIEditor* uiEditor) : Panel(editorSystemsRegistry),
                                         m_appSystemsRegistry(appSystemsRegistry),
                                         m_uiEditor(uiEditor),
                                         guizmo(id, appSystemsRegistry) {
        m_styleVars.emplace_back(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    }

    void UIView::onInit() {
        m_frameBufferID = m_appSystemsRegistry.getSystem<Renderer>().createFramebuffer(1080, 720);
        m_context = m_appSystemsRegistry.getSystem<Renderer>().getUIContext();
    }

    void UIView::onUpdate() {
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        ImVec2 viewPosition = ImGui::GetItemRectMin();
        FrameBuffer& frameBuffer = m_appSystemsRegistry.getSystem<Renderer>().getFramebuffer(m_frameBufferID);
        frameBuffer.bind();
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        frameBuffer.resize(wsize.x, wsize.y);
        m_context->SetDimensions(Rml::Vector2i(frameBuffer.width, frameBuffer.height));
        m_context->Update();
        UIRenderer& uiRenderer = m_appSystemsRegistry.getSystem<Renderer>().getUIRenderer();
        uiRenderer.onUpdate();
        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImGui::Image(reinterpret_cast<void*>(textureID), wsize, ImVec2(0, 1), ImVec2(1, 0));
        guizmo.editUI(viewPosition, wsize, m_uiEditor->getSelectedWidget());
        if (false && ImGui::IsWindowHovered()) {
            ImVec2 mousePos = ImGui::GetMousePos();
            ImVec2 imagePosition = ImGui::GetItemRectMin();
            m_lastMousePosition = ImVec2(mousePos.x - imagePosition.x, mousePos.y - imagePosition.y);

            m_context->ProcessMouseMove((int)m_lastMousePosition.x, (int)m_lastMousePosition.y, 0);

            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGuizmo::IsOver()) {
                Rml::Element* hoveredElement = m_context->GetHoverElement();
                if (hoveredElement && m_uiEditor->getElementToWidgetMap().find(hoveredElement) != m_uiEditor->getElementToWidgetMap().end()) {
                    Widget* widget = m_uiEditor->getElementToWidgetMap().at(hoveredElement);
                    if (widget) {
                        m_uiEditor->setSelectedWidget(widget);
                    }
                } else {
                    m_uiEditor->setSelectedWidget(nullptr);
                    m_uiEditor->setSelectedWidget(nullptr);
                }
            }
        }

        frameBuffer.unBind();
    }
}
