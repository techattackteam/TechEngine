#include "Panel.hpp"

#include "eventSystem/EventDispatcher.hpp"
#include "imgui_internal.h"
#include "core/Logger.hpp"

namespace TechEngine {
    Panel::Panel(SystemsRegistry& editorSystemsRegistry) : m_editorSystemsRegistry(editorSystemsRegistry) {
    }

    void Panel::init(const std::string& name, ImGuiWindowClass* parentDockSpaceClass, bool isVisible) {
        assert((!name.empty() && "Panel name is not set") || (parentDockSpaceClass != nullptr && "Parent dock space class is not set"));

        this->m_name = name;
        this->m_parentDockSpaceClass = parentDockSpaceClass;
        this->m_isVisible = isVisible;
        m_windowFlags |= ImGuiWindowFlags_NoCollapse;

        id = nextID++;
        registerShortcuts();
        onInit();
    }

    void Panel::registerShortcuts() {
    }

    void Panel::update() {
        assert((!this->m_name.empty() && "Panel name is not set") || (this->m_parentDockSpaceClass != nullptr && "Parent dock space class is not set"));
        ImGui::SetNextWindowClass(m_parentDockSpaceClass);
        if (!m_styleVars.empty()) {
            for (const auto& [styleVar, value]: m_styleVars) {
                if (std::holds_alternative<ImVec2>(value)) {
                    ImVec2 vector = std::get<ImVec2>(value);
                    ImGui::PushStyleVar(styleVar, vector);
                } else if (std::holds_alternative<float>(value)) {
                    float f = std::get<float>(value);
                    ImGui::PushStyleVar(styleVar, f);
                }
            }
        }

        ImGui::Begin((this->m_name + "##" + std::to_string(id)).c_str(), nullptr, m_windowFlags);
        ImVec2 currentSize = ImGui::GetContentRegionAvail();
        m_isResizing = (currentSize.x != m_lastSize.x || currentSize.y != m_lastSize.y);
        m_lastSize = currentSize;

        ImVec2 windowPos = ImGui::GetWindowPos();
        m_isMoving = (windowPos.x != m_lastPosition.x || windowPos.y != m_lastPosition.y);
        m_lastPosition = windowPos;

        this->onUpdate();
        processInput();
        if (m_isDragging) {
            m_windowFlags |= ImGuiWindowFlags_NoMove;
        } else {
            m_windowFlags &= ~ImGuiWindowFlags_NoMove;
        }

        if (!m_styleVars.empty()) {
            ImGui::PopStyleVar(m_styleVars.size());
        }
        ImGui::End();
    }


    void Panel::processInput() {
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigWindowsMoveFromTitleBarOnly = true;

        if (!m_isDragging &&
            ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) &&
            (ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right) || ImGui::IsMouseClicked(ImGuiMouseButton_Middle))) {
            ImGuiWindow* window = ImGui::GetCurrentWindow();

            float titleBarHeight = ImGui::GetFontSize() + 5.0f + window->WindowPadding.y * 4.0f;
            const float padding = 10.0f;

            ImVec2 draggingZoneMin = {window->Pos.x - padding, window->Pos.y + titleBarHeight};
            ImVec2 draggingZoneMax = {window->Pos.x + ImGui::GetContentRegionMax().x - padding, window->Pos.y + ImGui::GetContentRegionMax().y - padding};
            ImVec2 mousePos = ImGui::GetMousePos();

            bool isOnDraggingZone = (mousePos.x >= draggingZoneMin.x && mousePos.x <= draggingZoneMax.x &&
                                     mousePos.y >= draggingZoneMin.y && mousePos.y <= draggingZoneMax.y);

            if (isOnDraggingZone) {
                m_isDragging = true;
                m_isDragging = true;
            }
        }

        if (m_isDragging && !(ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsMouseDown(ImGuiMouseButton_Right) || ImGui::IsMouseDown(ImGuiMouseButton_Middle))) {
            m_isDragging = false;
        }

        if (m_isDragging) {
            glm::vec2 delta = glm::vec2(io.MouseDelta.x, io.MouseDelta.y);
            unsigned long long mouseButtons = 0;
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                mouseButtons |= MOUSE_1;
            if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
                mouseButtons |= MOUSE_2;
            if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
                mouseButtons |= MOUSE_3;
            processMouseDragging(delta, mouseButtons);
        }

        if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
            m_isPanelHovered = true;
            glm::vec2 delta = glm::vec2(io.MouseDelta.x, io.MouseDelta.y);
            processMouseMoving(delta);
        } else {
            m_isPanelHovered = false;
        }

        bool isWindowFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows | ImGuiFocusedFlags_RootAndChildWindows);
        float scrollY = io.MouseWheel;
        if ((m_isPanelHovered || isWindowFocused) && scrollY != 0.0f) {
            processMouseScroll(scrollY);
        }
    }

    void Panel::processShortcuts() {
    }

    void Panel::processMouseDragging(glm::vec2 delta, unsigned long long mouseButtons) {
    }

    void Panel::processMouseMoving(glm::vec2 delta) {
    }

    void Panel::processMouseScroll(float yOffset) {
    }
}
