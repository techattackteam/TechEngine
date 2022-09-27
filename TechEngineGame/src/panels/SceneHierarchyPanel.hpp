#pragma once

#include "CustomPanel.hpp"
#include "event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "event/events/gameObjects/GameObjectDestroyEvent.hpp"

namespace TechEngine {

    class SceneHierarchyPanel : public Panel {
    public:
        SceneHierarchyPanel();

        void onUpdate() override;
    };
}


