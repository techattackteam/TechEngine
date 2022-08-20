#pragma once

#include "CustomPanel.hpp"
#include "../events/gameObjects/GameObjectCreateEvent.hpp"
#include "../events/gameObjects/GameObjectDestroyEvent.hpp"

namespace TechEngine {

    class SceneHierarchyPanel : public Panel {
    public:
        SceneHierarchyPanel();

        void onUpdate() override;
    };
}


