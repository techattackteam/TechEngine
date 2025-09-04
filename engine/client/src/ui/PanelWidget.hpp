#pragma once
#include "Widget.hpp"

namespace TechEngine {
    class CLIENT_DLL PanelWidget : public Widget {
    private:
        glm::vec4 m_backgroundColor = {.5f, 0.5f, 0.5f, 1.0f};

    public:
        PanelWidget();

        void draw(UIRenderer& renderer) override;

        const glm::vec4& getBackgroundColor();

        void setBackgroundColor(const glm::vec4& color);
    };
}
