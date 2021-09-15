#pragma once

#include "ImGuiPanel.hpp"
#include "../scene/GameObject.hpp"

namespace Engine {
    class InspectorPanel : public ImGuiPanel {
    private:
        GameObject *gameObject;
    public:
        explicit InspectorPanel(GameObject *gameObject);

        void onUpdate() override;

    };
}

