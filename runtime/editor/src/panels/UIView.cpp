#include "UIView.hpp"

#include <imGuizmo.h>

#include "UIEditor.hpp"
#include "components/Components.hpp"
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
                                         cameraTransform(ComponentsFactory::createTransform(glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f))),
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

    void UIView::onKeyPressedEvent(Key& key) {
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
        Panel::onKeyPressedEvent(key);
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
        Panel::onKeyReleasedEvent(key);
    }

    void UIView::onMouseScrollEvent(float xOffset, float yOffset) {
        if (isWindowHovered && (lastUsingId == -1 || lastUsingId == id)) {
            const glm::mat4 inverted = glm::inverse(sceneCamera.getViewMatrix());
            const glm::vec3 forward = normalize(glm::vec3(inverted[2]));
            if (yOffset == -1.0f) {
                cameraTransform.translate(forward);
            } else if (yOffset == 1.0f) {
                cameraTransform.translate(-forward);
            }
        }
    }

    void UIView::onMouseMoveEvent(glm::vec2 delta) {
        if ((lastUsingId == -1 || lastUsingId == id) && (isWindowHovered || moving) && (mouse2 || mouse3)) {
            moving = true;
            lastUsingId = id;
            const glm::mat4 inverted = glm::inverse(sceneCamera.getViewMatrix());
            const glm::vec3 right = normalize(glm::vec3(inverted[0]));
            const glm::vec3 up = normalize(glm::vec3(inverted[1]));
            if (mouse3) {
                const glm::vec3 move = -right * delta.x * 0.01f + up * delta.y * 0.01f;
                cameraTransform.translate(move);
            }
            if (mouse2) {
                const glm::vec3 rotate = glm::vec3(-delta.y * 0.5f, -delta.x * 0.5f, 0);
                cameraTransform.rotate(rotate);
            }
        }
    }

    void UIView::drawHelperLines(ImVec2 imageTopLeft) {
        ImVec2 imageSize = ImGui::GetItemRectSize();

        for (const auto& pair: m_uiEditor->getElementToWidgetMap()) {
            std::shared_ptr<Widget> widget = pair.second;
            if (!widget) continue;
            Rml::Element* element = widget->getRmlElement();
            if (!element) continue;

            glm::ivec4 color = (widget == m_uiEditor->getSelectedWidget()) ? glm::ivec4(0, 255, 0, 255) : glm::ivec4(255, 0, 0, 255);

            Rml::Vector2f rmlPos = element->GetAbsoluteOffset(Rml::BoxArea::Border);
            Rml::Vector2f rmlSize = element->GetBox().GetSize(Rml::BoxArea::Border);

            ImVec2 topLeft = convertRmlToImGui(rmlPos, imageTopLeft, imageSize);
            ImVec2 bottomRight = convertRmlToImGui(rmlPos + rmlSize, imageTopLeft, imageSize);

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
