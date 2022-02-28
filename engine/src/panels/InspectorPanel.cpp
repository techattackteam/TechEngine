#include "InspectorPanel.hpp"

namespace Engine {
    InspectorPanel::InspectorPanel(GameObject *gameObject) : Panel("Inspector: " + gameObject->getName()) {
        this->gameObject = gameObject;
    }

    void InspectorPanel::onUpdate() {
        ImGui::Begin(name.c_str());
        gameObject->showInfo();
        ImGui::End();
    }

    GameObject *InspectorPanel::getGameObject() {
        return gameObject;
    }
}
