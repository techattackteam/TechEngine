#include "UIView.hpp"

#include <imGuizmo.h>

#include "UIEditor.hpp"
#include "renderer/FrameBuffer.hpp"
#include "renderer/Renderer.hpp"
#include "ui/Widget.hpp"
#include "systems/SystemsRegistry.hpp"


namespace TechEngine {
    // Constructor and onInit() remain the same...
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

        const float referenceHeight = 1080.0f; // Your UI is designed for a 1080p screen
        float dp_ratio = (wsize.y > 0) ? (wsize.y / referenceHeight) : 1.0f;
        m_context->SetDensityIndependentPixelRatio(dp_ratio);

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

        ImVec2 imageTopLeft = ImGui::GetItemRectMin();
        ImVec2 imageSize = ImGui::GetItemRectSize();
        guizmo.editUI(imageTopLeft, imageSize, m_uiEditor->getSelectedWidget());
        drawHelperLines(imageTopLeft);
        frameBuffer.unBind();
    }

    void UIView::drawHelperLines(ImVec2 imageTopLeft) {
        for (auto pair: m_uiEditor->getElementToWidgetMap()) {
            std::shared_ptr<Widget> widget = pair.second;
            glm::ivec4 color;
            if (widget == m_uiEditor->getSelectedWidget()) {
                color = glm::vec4(0, 255, 0, 255); // Green for selected widget
            } else {
                color = glm::vec4(255, 0, 0, 255); // Red for other widgets
            }
            if (!widget) return;
            auto* element = widget->getRmlElement();
            if (!element) return;

            auto rmlPos = element->GetAbsoluteOffset(Rml::BoxArea::Border);
            auto rmlSize = element->GetBox().GetSize(Rml::BoxArea::Border);

            ImVec2 rectTopLeft = {
                imageTopLeft.x + rmlPos.x,
                imageTopLeft.y + (rmlPos.y + rmlSize.y)
            };
            ImVec2 rectBottomRight = {
                rectTopLeft.x + rmlSize.x,
                imageTopLeft.y + rmlPos.y
            };

            ImGui::GetForegroundDrawList()->AddRect(
                rectTopLeft, rectBottomRight,
                IM_COL32(
                    color.r,
                    color.g,
                    color.b,
                    color.a
                ),
                0.0f, ImDrawFlags_None, 1.0f);
        }
    }
}
