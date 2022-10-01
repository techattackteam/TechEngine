#include "InspectorPanel.hpp"

namespace TechEngine {
    InspectorPanel::InspectorPanel() : Panel("Inspector") {
        TechEngineCore::EventDispatcher::getInstance().subscribe(OnSelectGameObjectEvent::eventType, [this](TechEngineCore::Event *event) {
            inspectGameObject((OnSelectGameObjectEvent *) (event));
        });
    }


    void InspectorPanel::onUpdate() {
        ImGui::Begin(name.c_str());
        if (gameObject != nullptr) {
        }
        ImGui::End();
    }

    void InspectorPanel::inspectGameObject(OnSelectGameObjectEvent *event) {
        this->gameObject = event->getGameObject();
    }

}
