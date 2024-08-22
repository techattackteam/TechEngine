#include "Panel.hpp"

namespace TechEngine {
    void Panel::init(const std::string& name, ImGuiWindowClass* parentDockSpaceClass, bool isVisible) {
        assert((!name.empty() && "Panel name is not set") || (parentDockSpaceClass != nullptr && "Parent dock space class is not set"));

        this->m_name = name;
        this->m_parentDockSpaceClass = parentDockSpaceClass;
        this->m_isVisible = isVisible;
        m_windowFlags |= ImGuiWindowFlags_NoCollapse;

        onInit();
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
        ImGui::Begin(this->m_name.c_str(), nullptr, m_windowFlags);
        if (!m_styleVars.empty()) {
            ImGui::PopStyleVar(m_styleVars.size());
        }
        this->onUpdate();
        ImGui::End();
    }
}
