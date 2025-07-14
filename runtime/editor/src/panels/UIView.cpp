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
        ImVec2 imageSize = ImGui::GetItemRectSize();

        for (const auto& pair: m_uiEditor->getElementToWidgetMap()) {
            std::shared_ptr<Widget> widget = pair.second;
            if (!widget) continue;
            Rml::Element* element = widget->getRmlElement();
            if (!element) continue;

            // Color
            glm::ivec4 color = (widget == m_uiEditor->getSelectedWidget()) ? glm::ivec4(0, 255, 0, 255) : glm::ivec4(255, 0, 0, 255);

            // Get RML positions
            Rml::Vector2f rmlPos = element->GetAbsoluteOffset(Rml::BoxArea::Border);
            Rml::Vector2f rmlSize = element->GetBox().GetSize(Rml::BoxArea::Border);

            // Compute top-left and bottom-right
            ImVec2 topLeft = convertRmlToImGui(rmlPos, imageTopLeft, imageSize);
            ImVec2 bottomRight = convertRmlToImGui(rmlPos + rmlSize, imageTopLeft, imageSize);
            // Draw widget box
            ImGui::GetForegroundDrawList()->AddRect(
                topLeft, bottomRight,
                IM_COL32(color.r, color.g, color.b, color.a), 0.0f, ImDrawFlags_None, 1.5f);
            if (pair.second == m_uiEditor->getSelectedWidget()) {

                // Draw pivot cross
                Rml::Vector2f pivotOffset = {
                    rmlPos.x + widget->m_pivot.x * rmlSize.x,
                    rmlPos.y + widget->m_pivot.y * rmlSize.y
                };
                ImVec2 pivotScreen = convertRmlToImGui(pivotOffset, imageTopLeft, imageSize);
                ImGui::GetForegroundDrawList()->AddLine(
                    {pivotScreen.x - 5, pivotScreen.y}, {pivotScreen.x + 5, pivotScreen.y}, IM_COL32(255, 255, 0, 255));
                ImGui::GetForegroundDrawList()->AddLine(
                    {pivotScreen.x, pivotScreen.y - 5}, {pivotScreen.x, pivotScreen.y + 5}, IM_COL32(255, 255, 0, 255));

                // Draw anchor min/max (optional)
                Rml::Vector2f parentSize = element->GetParentNode() ? element->GetParentNode()->GetBox().GetSize(Rml::BoxArea::Content) : Rml::Vector2f(1920, 1080); // Fallback

                Rml::Vector2f anchorMinPos = element->GetParentNode()->GetAbsoluteOffset() +
                                             Rml::Vector2f(widget->m_anchorMin.x * parentSize.x, widget->m_anchorMin.y * parentSize.y);
                Rml::Vector2f anchorMaxPos = element->GetParentNode()->GetAbsoluteOffset() +
                                             Rml::Vector2f(widget->m_anchorMax.x * parentSize.x, widget->m_anchorMax.y * parentSize.y);

                ImVec2 anchorMin = convertRmlToImGui(anchorMinPos, imageTopLeft, imageSize);
                ImVec2 anchorMax = convertRmlToImGui(anchorMaxPos, imageTopLeft, imageSize);

                ImGui::GetForegroundDrawList()->AddRect(
                    anchorMin, anchorMax, IM_COL32(0, 128, 255, 128), 0.0f, ImDrawFlags_None, 1.0f);
            }
        }
    }


    ImVec2 UIView::convertRmlToImGui(const Rml::Vector2f& rmlPos, const ImVec2& imageTopLeft, const ImVec2& imageSize) {
        // Get the context size
        const Rml::Vector2f contextSize = Rml::Vector2f(m_context->GetDimensions());

        // Normalize RML position to [0, 1] range
        Rml::Vector2f normalized = {
            rmlPos.x / contextSize.x,
            rmlPos.y / contextSize.y
        };

        // Convert to ImGui space inside image panel
        return {
            imageTopLeft.x + normalized.x * imageSize.x,
            imageTopLeft.y + normalized.y * imageSize.y
        };
    }
}
