#pragma once

#include "CustomPanel.hpp"
#include "scene/GameObject.hpp"
#include "events/OnSelectGameObjectEvent.hpp"
#include "event/events/gameObjects/GameObjectDestroyEvent.hpp"

namespace TechEngine {
    class InspectorPanel : public Panel {
    private:
    public:
        explicit InspectorPanel();

        void onUpdate() override;

        template<typename T, typename UIFunction>
        void drawComponent(const std::string &name, UIFunction uiFunction);

        void drawComponents();

        void drawVec3Control(const std::string &label, glm::vec3 &values, float resetValue = 0.0f, float columnWidth = 100.0f, float min = 0.0f, float max = 0.0f);

    };
}

