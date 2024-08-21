#include "DockPanel.hpp"


namespace TechEngine {
    void DockPanel::onInit() {
        m_windowFlags |= ImGuiWindowFlags_NoBackground;
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
        ImGui::Begin(this->m_name.c_str(), nullptr, m_windowFlags);
        ImGui::PopStyleVar();
        if (m_styleVars.size() > 0) {
            ImGui::PopStyleVar(m_styleVars.size());
        }
        std::string dockSpaceName = this->m_name + "DockSpace";
        ImGuiID dockSpaceID = ImGui::GetID(dockSpaceName.c_str());
        m_dockSpaceWindowClass.ClassId = dockSpaceID;
        ImGuiDockNodeFlags dockSpaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;
        ImGui::DockSpace(dockSpaceID, ImVec2(0.0f, 0.0f), dockSpaceFlags, &m_dockSpaceWindowClass);
        this->onUpdate();
        ImGui::End();
    }
}
