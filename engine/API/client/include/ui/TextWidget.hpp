#pragma once

#include <glm/vec4.hpp>

#include "Widget.hpp"

namespace TechEngine {
    class TextWidget;
}

namespace TechEngineAPI {
    class API_DLL TextWidget : public Widget {
    public:
        explicit TextWidget(std::shared_ptr<TechEngine::TextWidget>& widget);

        std::string& getText();

        void setText(const std::string& text);

        glm::vec4& getColor();
    };
}
