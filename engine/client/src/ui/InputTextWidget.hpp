#pragma once

#include "Widget.hpp"

namespace TechEngine {
    class SystemsRegistry;
}

namespace TechEngine {
    class Key;

    class CLIENT_DLL InputTextWidget : public Widget {
    private:
        SystemsRegistry& m_systemsRegistry;

        std::string m_text;
        std::string m_placeholderText = "Enter text...";
        bool m_isFocused = false;

        float m_cursorBlinkTimer = 0.0f;
        bool m_isCursorVisible = true;

        glm::vec4 m_textColor = {1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 m_placeholderColor = {0.6f, 0.6f, 0.6f, 1.0f};
        glm::vec4 m_backgroundColor = {0.1f, 0.1f, 0.1f, 1.0f};
        glm::vec4 m_borderColor = {0.3f, 0.3f, 0.3f, 1.0f};
        glm::vec4 m_focusedBorderColor = {0.2f, 0.6f, 1.0f, 1.0f};
        float m_fontSize = 18.0f; // In reference pixels

    public:
        InputTextWidget(SystemsRegistry& systemsRegistry);

        ~InputTextWidget() override;

        void update(float deltaTime) override;

        void draw(UIRenderer& renderer) override;

        void gainFocus();

        void loseFocus();

        void onKeyPressed(Key& keyCode);
    };
}
