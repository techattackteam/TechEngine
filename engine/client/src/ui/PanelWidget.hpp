#pragma once
#include "Widget.hpp"

namespace TechEngine {
    class CLIENT_DLL PanelWidget : public Widget {
    public:
        glm::vec4 m_backgroundColor = {1.0f, 1.0f, 1.0f, 1.0f};
    public:
        PanelWidget();

        void draw(UIRenderer& renderer) override;
    };
}
