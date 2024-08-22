#include "DockPanel.hpp"

#include <filesystem>
#include <imgui_internal.h>


namespace TechEngine {
    void DockPanel::onInit() {
    }


    void DockPanel::update() {
        assert((!this->m_name.empty() && "Panel name is not set") || (this->m_parentDockSpaceClass != nullptr && "Parent dock space class is not set"));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
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
        ImGui::SetNextWindowClass(m_parentDockSpaceClass);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::Begin(this->m_name.c_str(), nullptr, m_windowFlags);
        ImGui::PopStyleVar();
        if (m_styleVars.size() > 0) {
            ImGui::PopStyleVar(m_styleVars.size());
        }
        std::string dockSpaceName = this->m_name + "DockSpace";
        m_dockSpaceID = ImGui::GetID(dockSpaceName.c_str());
        m_dockSpaceWindowClass.ClassId = m_dockSpaceID;
        ImGuiDockNodeFlags dockSpaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;

        createToolBar();

        ImGui::DockSpace(m_dockSpaceID, ImVec2(0.0f, 0.0f), dockSpaceFlags, &m_dockSpaceWindowClass);
        ImGui::PopStyleColor();
        this->onUpdate();
        if (m_firstTime) {
            if (!std::filesystem::exists("imgui.ini")) {
                setupInitialDockingLayout();
            }
            m_firstTime = false;
        }
        ImGui::End();
    }

    void DockPanel::createToolBar() {
    }
}
