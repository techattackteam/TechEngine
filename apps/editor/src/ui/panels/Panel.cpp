#include <ui/panels/Panel.hpp>

#include <imgui.h>

#include <utility>

namespace TechEngine {
    Panel::Panel(std::string name) : m_name(std::move(name)) {
    }

    void Panel::draw(const EditorSnapshot& snapshot) {
        const bool visible = ImGui::Begin(m_name.c_str(), nullptr, ImGuiWindowFlags_NoCollapse);
        if (visible) {
            onDraw(snapshot);
        }
        // Unlike BeginMenu or BeginPopup, Begin needs its End even when it returns false.
        ImGui::End();
    }

    const std::string& Panel::getName() const {
        return m_name;
    }
}
