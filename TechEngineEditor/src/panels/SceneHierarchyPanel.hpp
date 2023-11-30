#pragma once

#include "CustomPanel.hpp"
#include "scene/Scene.hpp"
#include "material/MaterialManager.hpp"

namespace TechEngine {

    class SceneHierarchyPanel : public Panel {
    private:
        std::vector<GameObject *> selectedGO;
        MaterialManager &materialManager;
        Scene &scene;
        bool isItemHovered = false;
        bool isShiftPressed = false;
        bool isCtrlPressed = false;

    public:
        SceneHierarchyPanel(Scene &scene, MaterialManager &materialManager);

        void onUpdate() override;

        void drawEntityNode(GameObject *gameObject);

        std::vector<GameObject *> &getSelectedGO();

        void selectGO(GameObject *selectedGO);

        void deselectGO(GameObject *selectedGO);

    private:
        void deleteGameObject(GameObject *gameObject);

        void onKeyPressedEvent(Key &key) override;

        void onKeyReleasedEvent(Key &key) override;

    };
}


