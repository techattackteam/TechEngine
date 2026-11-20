#include <ui/panels/Panel.hpp>

#include <imgui.h>

#include <utility>
#include <variant>

namespace TechEngine {
    Panel::Panel(std::string name) : m_name(std::move(name)) {
    }

    bool Panel::draw(const EditorSnapshot& snapshot) {
        for (const PanelStyleVar& styleVar: m_styleVars) {
            std::visit(
                [&](const auto& value) {
                    ImGui::PushStyleVar(styleVar.var, value);
                },
                styleVar.value);
        }
        const bool visible = ImGui::Begin(m_name.c_str(), nullptr, ImGuiWindowFlags_NoCollapse);
        if (visible) {
            onDraw(snapshot);
        }
        ImGui::PopStyleVar(static_cast<int>(m_styleVars.size()));
        // Unlike BeginMenu or BeginPopup, Begin needs its End even when it returns false.
        ImGui::End();
        return visible;
    }

    void Panel::addStyleVar(const ImGuiStyleVar var, const float value) {
        m_styleVars.push_back(PanelStyleVar{var, value});
    }

    void Panel::addStyleVar(const ImGuiStyleVar var, const ImVec2 value) {
        m_styleVars.push_back(PanelStyleVar{var, value});
    }

    const std::string& Panel::getName() const {
        return m_name;
    }
}
