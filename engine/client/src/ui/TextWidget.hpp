#pragma once
#include "Widget.hpp"

namespace TechEngine {
    class CLIENT_DLL TextWidget : public Widget {
    public:
        std::string m_text = "Default Text";
        glm::vec4 m_textColor = {1.0f, 1.0f, 1.0f, 1.0f};
        float m_fontSize = 18.0f; // In reference pixels
        bool m_isBold;
        bool m_isItalic;

    public:
        TextWidget();

        void draw(UIRenderer& renderer) override;

        std::string& getText();


        void setText(const std::string& text);


        glm::vec4& getColor();

        float& getFontSize();


        bool& isBold();

        bool& isItalic();

        int getHorizontalAlign() {
            return 0;
        }

        int getVerticalAlign() {
            return 0;
        }
    };
}
