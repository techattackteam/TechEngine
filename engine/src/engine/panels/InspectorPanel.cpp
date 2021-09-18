#include "InspectorPanel.hpp"

namespace Engine {
    InspectorPanel::InspectorPanel(GameObject *gameObject) : ImGuiPanel("Inspector: " + gameObject->getName()) {
        this->gameObject = gameObject;
    }

    void InspectorPanel::onUpdate() {
        ImGui::Begin(name.c_str());
        gameObject->showInfo();
        ImGui::End();
    }
}
