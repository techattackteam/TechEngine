#pragma once

#include "CustomPanel.hpp"
#include "event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "event/events/gameObjects/GameObjectDestroyEvent.hpp"
#include "scene/Scene.hpp"

namespace TechEngine {

    class SceneHierarchyPanel : public Panel {
    private:
        GameObject *selectedGO = nullptr;
        Scene &scene = Scene::getInstance();
        bool isItemHovered = false;
    public:
        SceneHierarchyPanel();

        void onUpdate() override;

        void drawEntityNode(GameObject *gameObject);
    };
}


