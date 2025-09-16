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
        Renderer& renderer = m_appSystemsRegistry.getSystem<Renderer>();
        FrameBuffer& frameBuffer = m_appSystemsRegistry.getSystem<Renderer>().getFramebuffer(m_frameBufferID);
        frameBuffer.bind();

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
            renderer.getUIRenderer().setViewport(gameViewSize.x, gameViewSize.y);
        }

        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderer.uiPass();

        uint64_t textureID = frameBuffer.getColorAttachmentRenderer();
        ImVec2 canvasSize = {(float)gameViewSize.x, (float)gameViewSize.y};
        ImVec2 panelTopLeft = ImGui::GetCursorScreenPos();

        ImVec2 canvasTopLeftOnScreen = {panelTopLeft.x + m_panOffset.x, panelTopLeft.y + m_panOffset.y};
        ImVec2 canvasBottomRightOnScreen = {canvasTopLeftOnScreen.x + canvasSize.x * m_zoom, canvasTopLeftOnScreen.y + canvasSize.y * m_zoom};

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
        drawHelperLines(canvasTopLeftOnScreen);
        frameBuffer.unBind();
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

    void UIView::drawHelperLines(ImVec2 canvasTopLeftOnScreen) {
        for (auto& widget: m_uiEditor->getWidgetsRegistry().getWidgets()) {
            if (!widget) {
                return;
            }

            const glm::vec4& widgetRect = widget->m_finalScreenRect; // {x, y, width, height}
            const glm::vec2& pivot = widget->m_pivot; // {px, py} normalized

            ImVec2 boxTopLeft = {
                canvasTopLeftOnScreen.x + (widgetRect.x * m_zoom),
                canvasTopLeftOnScreen.y + (widgetRect.y * m_zoom)
            };

            ImVec2 boxBottomRight = {
                boxTopLeft.x + (widgetRect.z * m_zoom),
                boxTopLeft.y + (widgetRect.w * m_zoom)
            };

            float pivotUiX = widgetRect.x + (widgetRect.z * pivot.x);
            float pivotUiY = widgetRect.y + (widgetRect.w * pivot.y);

            ImVec2 pivotScreenPos = {
                canvasTopLeftOnScreen.x + (pivotUiX * m_zoom),
                canvasTopLeftOnScreen.y + (pivotUiY * m_zoom)
            };

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const ImU32 boxColor = widget == m_uiEditor->getSelectedWidget() ? IM_COL32(255, 255, 255, 255) : IM_COL32(150, 150, 150, 100);
            const ImU32 pivotColor = IM_COL32(255, 255, 0, 255);
            const float crosshairSize = 6.0f;
            const float lineThickness = 1.5f;

            drawList->AddRect(boxTopLeft, boxBottomRight, boxColor, 0.0f, ImDrawFlags_None, lineThickness);
            if (widget == m_uiEditor->getSelectedWidget()) {
                drawList->AddLine(
                    {pivotScreenPos.x - crosshairSize, pivotScreenPos.y},
                    {pivotScreenPos.x + crosshairSize, pivotScreenPos.y},
                    pivotColor,
                    lineThickness
                );
                drawList->AddLine(
                    {pivotScreenPos.x, pivotScreenPos.y - crosshairSize},
                    {pivotScreenPos.x, pivotScreenPos.y + crosshairSize},
                    pivotColor,
                    lineThickness
                );
            }
        }
    }
}
