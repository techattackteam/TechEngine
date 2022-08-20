#pragma once

#include "CustomPanel.hpp"
#include "../scene/GameObject.hpp"

namespace TechEngine {
    class InspectorPanel : public Panel {
    private:
        GameObject *gameObject;
    public:
        explicit InspectorPanel(GameObject *gameObject);

        void onUpdate() override;

        GameObject *getGameObject();

    };
}

