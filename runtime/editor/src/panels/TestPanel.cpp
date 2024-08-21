#include "TestPanel.hpp"

void TechEngine::TestPanel::onInit() {
    m_styleVars.emplace_back(ImGuiStyleVar_WindowPadding, ImVec2(100, 100));
    m_styleVars.emplace_back(ImGuiStyleVar_WindowRounding, 20.0f);
}

void TechEngine::TestPanel::onUpdate() {
    ImGui::Text("Hello, World!");
}
