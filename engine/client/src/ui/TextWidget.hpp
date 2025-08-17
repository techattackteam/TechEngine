#pragma once
#include "Widget.hpp"

namespace TechEngine {
    class CLIENT_DLL TextWidget : public Widget {
    public:
        std::string m_text = "Default Text";
        glm::vec4 m_textColor = {1.0f, 1.0f, 1.0f, 1.0f};

    public:
        TextWidget();

        void draw(UIRenderer& renderer);
    };
}
