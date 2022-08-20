#include <ios>
#include "SceneHierarchyPanel.hpp"
#include "../scene/Scene.hpp"
#include "../events/panels/CloseInspectorEvent.hpp"
#include "../events/panels/OpenInspectorEvent.hpp"

namespace TechEngine {
    SceneHierarchyPanel::SceneHierarchyPanel() : Panel("SceneHierarchyPanel") {

    }

    void SceneHierarchyPanel::onUpdate() {
        Scene &scene = Scene::getInstance();
        if (!scene.getGameObjects().empty()) {
            ImGui::Begin(name.c_str());
            for (auto element: Scene::getInstance().getGameObjects()) {
                std::string text = element->getName() + " " + (element->showInfoPanel ? "true" : "false");
                if (ImGui::Button(text.c_str())) {
                    if (element->showInfoPanel) {
                        TechEngineCore::EventDispatcher::getInstance().dispatch(new CloseInspectorEvent(element));
                    } else {
                        TechEngineCore::EventDispatcher::getInstance().dispatch(new OpenInspectorEvent(element));
                    }
                    element->showInfoPanel = !element->showInfoPanel;
                }
            }
            ImGui::End();
        }
    }
}
