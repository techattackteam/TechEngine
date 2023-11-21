#pragma once

#include "CustomPanel.hpp"
#include "scene/GameObject.hpp"
#include "event/events/gameObjects/GameObjectDestroyEvent.hpp"

namespace TechEngine {
    class InspectorPanel : public Panel {
    private:
    public:
        explicit InspectorPanel();

        void onUpdate() override;

        template<typename T, typename UIFunction>
        void drawComponent(GameObject *gameObject, const std::string &name, UIFunction uiFunction);

        void drawComponents();

    private:
        template<typename C>
        void addComponent();

        void drawCommonComponents();
    };
}

