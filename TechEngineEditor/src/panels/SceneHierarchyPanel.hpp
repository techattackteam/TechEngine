#pragma once

#include "CustomPanel.hpp"
#include "event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "event/events/gameObjects/GameObjectDestroyEvent.hpp"

namespace TechEngine {

    class SceneHierarchyPanel : public Panel {
    private:
        GameObject* selectedGO;
    public:
        SceneHierarchyPanel();

        void onUpdate() override;

        void DrawVec3Control(const std::string &label, glm::vec3 &values, float resetValue, float columnWidth);

        void drawEntityNode(GameObject *gameObject);
    };
}


