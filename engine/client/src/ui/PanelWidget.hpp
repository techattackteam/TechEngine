#pragma once
#include "Widget.hpp"

namespace TechEngine {
    class PanelWidget : public Widget {
    public:
        glm::vec4 m_backgroundColor = {0.2f, 0.2f, 0.2f, 1.0f}; // Default background color
    public:
        PanelWidget();

        void draw(UIRenderer& renderer) override;
    };
}
