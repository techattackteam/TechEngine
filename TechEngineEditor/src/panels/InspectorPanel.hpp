#pragma once

#include "CustomPanel.hpp"
#include "scene/GameObject.hpp"
#include "events/OnSelectGameObjectEvent.hpp"

namespace TechEngine {
    class InspectorPanel : public Panel {
    private:
        GameObject *gameObject = nullptr;
    public:
        explicit InspectorPanel();

        void onUpdate() override;

        void inspectGameObject(OnSelectGameObjectEvent *event);
    };
}

