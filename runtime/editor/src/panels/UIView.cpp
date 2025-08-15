#include "UIView.hpp"

#include <imGuizmo.h>

#include "GameView.hpp"
#include "SceneView.hpp"
#include "UIEditor.hpp"
#include "components/ComponentsFactory.hpp"
#include "renderer/FrameBuffer.hpp"
#include "renderer/Renderer.hpp"
#include "ui/Widget.hpp"
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
    }


    void UIView::onUpdate() {
        glm::vec2 gameViewSize = m_uiEditor->getGameView().getFrameBufferSize();
        FrameBuffer& frameBuffer = m_appSystemsRegistry.getSystem<Renderer>().getFramebuffer(m_frameBufferID);
        UIRenderer& uiRenderer = m_appSystemsRegistry.getSystem<Renderer>().getUIRenderer();

        if (m_isPanelHovered && m_pendingScrollY != 0.0f) {
            float oldZoom = m_zoom;

            m_zoom += m_pendingScrollY * 0.1f * m_zoom;
            m_zoom = std::max(m_zoom, 0.1f);
            m_zoom = std::min(m_zoom, 10.0f);

            ImVec2 panelTopLeft = ImGui::GetCursorScreenPos();
            ImVec2 mousePosInPanel = {ImGui::GetIO().MousePos.x - panelTopLeft.x, ImGui::GetIO().MousePos.y - panelTopLeft.y};

            if (m_pendingScrollY > 0) {
                m_panOffset.x = mousePosInPanel.x - (mousePosInPanel.x - m_panOffset.x) * (m_zoom / oldZoom);
                m_panOffset.y = mousePosInPanel.y - (mousePosInPanel.y - m_panOffset.y) * (m_zoom / oldZoom);
            } else {
                m_panOffset.x = mousePosInPanel.x - (mousePosInPanel.x - m_panOffset.x) * (m_zoom / oldZoom);
                m_panOffset.y = mousePosInPanel.y - (mousePosInPanel.y - m_panOffset.y) * (m_zoom / oldZoom);
            }
        }
        m_pendingScrollY = 0.0f;

        if (gameViewSize.x > 0 && gameViewSize.y > 0 &&
            (frameBuffer.width != gameViewSize.x || frameBuffer.height != gameViewSize.y)) {
            frameBuffer.resize(gameViewSize.x, gameViewSize.y);
            uiRenderer.setViewport(gameViewSize.x, gameViewSize.y);
        }
        //m_context->SetDimensions(Rml::Vector2i(gameViewSize.x, gameViewSize.y));

        const float referenceHeight = 1080.0f;
        float dp_ratio = (gameViewSize.y > 0) ? (gameViewSize.y / referenceHeight) : 1.0f;
        //m_context->SetDensityIndependentPixelRatio(dp_ratio);


        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //m_context->Update();
        frameBuffer.bind();
        uiRenderer.beginFrame();
        uiRenderer.endFrame();
        frameBuffer.unBind();

        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImVec2 canvasSize = {(float)gameViewSize.x, (float)gameViewSize.y};
        ImVec2 panelTopLeft = ImGui::GetCursorScreenPos();

        ImVec2 canvasTopLeftOnScreen = {panelTopLeft.x + m_panOffset.x, panelTopLeft.y + m_panOffset.y};
        ImVec2 canvasBottomRightOnScreen = {canvasTopLeftOnScreen.x + canvasSize.x * m_zoom, canvasTopLeftOnScreen.y + canvasSize.y * m_zoom};

        // Use a draw list to render the image with precise control
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        draw_list->AddImage(
            reinterpret_cast<void*>(textureID),
            canvasTopLeftOnScreen,
            canvasBottomRightOnScreen,
            ImVec2(0, 1), // UV0 (top-left)
            ImVec2(1, 0) // UV1 (bottom-right) -> Flipped vertically for OpenGL textures
        );

        ImVec2 imageTopLeft = ImGui::GetItemRectMin();
        ImVec2 imageSize = ImGui::GetItemRectSize();
        guizmo.editUI(imageTopLeft, imageSize, m_uiEditor->getSelectedWidget());
        //drawHelperLines(imageTopLeft);
    }

    /*void UIView::onKeyPressedEvent(Key& key) {
        switch (key.getKeyCode()) {
            case MOUSE_2: {
                mouse2 = true;
                break;
            }
            case MOUSE_3: {
                mouse3 = true;
                break;
            }
        }
    }

    void UIView::onKeyReleasedEvent(Key& key) {
        switch (key.getKeyCode()) {
            case MOUSE_2: {
                mouse2 = false;
                moving = false;
                lastUsingId = -1;
                break;
            }
            case MOUSE_3: {
                mouse3 = false;
                moving = false;
                lastUsingId = -1;
                break;
            }
        }
    }*/

    void UIView::processMouseDragging(glm::vec2 delta, unsigned long long mouseButtons) {
        if (m_isPanelHovered && mouseButtons & MOUSE_3) { // Use middle mouse button to pan
            m_panOffset.x += delta.x;
            m_panOffset.y += delta.y;
        }
    }

    void UIView::processMouseScroll(float yOffset) {
        if (m_isPanelHovered) {
            m_pendingScrollY += yOffset;
        }
    }

    void UIView::drawHelperLines(ImVec2 imageTopLeft) {
        /*ImVec2 imageSize = ImGui::GetItemRectSize();

        for (const auto& pair: m_uiEditor->getElementToWidgetMap()) {
            std::shared_ptr<Widget> widget = pair.second;
            if (!widget) continue;
            Rml::Element* element = widget->getRmlElement();
            if (!element) continue;

            glm::ivec4 color = (widget == m_uiEditor->getSelectedWidget()) ? glm::ivec4(0, 255, 0, 255) : glm::ivec4(255, 0, 0, 255);

            glm::vec2 rmlPos = element->GetAbsoluteOffset(Rml::BoxArea::Border);
            glm::vec2 rmlSize = element->GetBox().GetSize(Rml::BoxArea::Border);

            ImVec2 topLeft = convertRmlToImGui(rmlPos, imageTopLeft, imageSize);
            ImVec2 bottomRight = convertRmlToImGui(rmlPos + rmlSize, imageTopLeft, imageSize);

            ImGui::GetForegroundDrawList()->AddRect(
                topLeft, bottomRight,
                IM_COL32(color.r, color.g, color.b, color.a), 0.0f, ImDrawFlags_None, 1.5f);
            if (pair.second == m_uiEditor->getSelectedWidget()) {
                // Draw pivot cross
                glm::vec2 pivotOffset = {
                    rmlPos.x + widget->m_pivot.x * rmlSize.x,
                    rmlPos.y + widget->m_pivot.y * rmlSize.y
                };
                ImVec2 pivotScreen = convertRmlToImGui(pivotOffset, imageTopLeft, imageSize);
                ImGui::GetForegroundDrawList()->AddLine(
                    {pivotScreen.x - 5, pivotScreen.y}, {pivotScreen.x + 5, pivotScreen.y}, IM_COL32(255, 255, 0, 255));
                ImGui::GetForegroundDrawList()->AddLine(
                    {pivotScreen.x, pivotScreen.y - 5}, {pivotScreen.x, pivotScreen.y + 5}, IM_COL32(255, 255, 0, 255));

                // Draw anchor min/max (optional)
                glm::vec2 parentSize = element->GetParentNode() ? element->GetParentNode()->GetBox().GetSize(Rml::BoxArea::Content) : glm::vec2(1920, 1080); // Fallback

                glm::vec2 anchorMinPos = element->GetParentNode()->GetAbsoluteOffset() +
                                         glm::vec2(widget->m_anchorMin.x * parentSize.x, widget->m_anchorMin.y * parentSize.y);
                glm::vec2 anchorMaxPos = element->GetParentNode()->GetAbsoluteOffset() +
                                         glm::vec2(widget->m_anchorMax.x * parentSize.x, widget->m_anchorMax.y * parentSize.y);

                ImVec2 anchorMin = convertRmlToImGui(anchorMinPos, imageTopLeft, imageSize);
                ImVec2 anchorMax = convertRmlToImGui(anchorMaxPos, imageTopLeft, imageSize);

                ImGui::GetForegroundDrawList()->AddRect(
                    anchorMin, anchorMax, IM_COL32(0, 128, 255, 128), 0.0f, ImDrawFlags_None, 1.0f);
            }
        }*/
    }


    /*ImVec2 UIView::convertRmlToImGui(const glm::vec2& rmlPos, const ImVec2& imageTopLeft, const ImVec2& imageSize) {
        /#1#/ Get the context size
        const glm::vec2 contextSize = glm::vec2(m_context->GetDimensions());

        // Normalize RML position to [0, 1] range
        glm::vec2 normalized = {
            rmlPos.x / contextSize.x,
            rmlPos.y / contextSize.y
        };

        // Convert to ImGui space inside image panel
        return {
            imageTopLeft.x + normalized.x * imageSize.x,
            imageTopLeft.y + normalized.y * imageSize.y
        };#1#
    }

    ImVec2 UIView::convertRmlToEditorScreen(const glm::vec2& rmlPos, const ImVec2& panelTopLeft) {
        /*glm::vec2 gameViewSize = m_uiEditor->getGameView().getFrameBufferSize();

        // 1. Position on the canvas (0 to gameViewSize)
        float canvasX = rmlPos.x * m_zoom;
        float canvasY = rmlPos.y * m_zoom;

        // 2. Final screen position including pan and panel offset
        return {
            panelTopLeft.x + m_panOffset.x + canvasX,
            panelTopLeft.y + m_panOffset.y + canvasY
        };#1#
        return vec2;
    }

    // You will need the inverse for mouse picking (e.g., to select a widget)
    glm::vec2 UIView::convertEditorScreenToRml(const ImVec2& screenPos, const ImVec2& panelTopLeft) {
        /*glm::vec2 gameViewSize = m_uiEditor->getGameView().getFrameBufferSize();

        // 1. Position relative to the panned canvas's top-left
        float canvasX = screenPos.x - panelTopLeft.x - m_panOffset.x;
        float canvasY = screenPos.y - panelTopLeft.y - m_panOffset.y;

        // 2. Normalize back to RML coordinates
        return {
            canvasX / m_zoom,
            canvasY / m_zoom
        };#1#
    }*/
}
