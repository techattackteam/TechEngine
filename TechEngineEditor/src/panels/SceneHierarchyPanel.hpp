#pragma once

#include "CustomPanel.hpp"
#include "event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "event/events/gameObjects/GameObjectDestroyEvent.hpp"
#include "scene/Scene.hpp"

namespace TechEngine {

    class SceneHierarchyPanel : public Panel {
    private:
        std::list<GameObject *> selectedGO;
        Scene &scene = Scene::getInstance();
        bool isItemHovered = false;
        bool isShiftPressed = false;
        bool isCtrlPressed = false;

    public:
        SceneHierarchyPanel();

        void onUpdate() override;

        void drawEntityNode(GameObject *gameObject);


        std::list<GameObject *> &getSelectedGO();

        void selectGO(GameObject *selectedGO);

        void deselectGO(GameObject *selectedGO);


    private:
        void deleteGameObject(GameObject *gameObject);

        void onKeyPressedEvent(Key &key) override;

        void onKeyReleasedEvent(Key &key) override;

    };
}


