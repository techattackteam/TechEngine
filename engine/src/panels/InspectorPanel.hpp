#pragma once

#include "CustomPanel.hpp"
#include "../scene/GameObject.hpp"

namespace Engine {
    class InspectorPanel : public Panel {
    private:
        GameObject *gameObject;
    public:
        explicit InspectorPanel(GameObject *gameObject);

        void onUpdate() override;

        GameObject *getGameObject();

    };
}

