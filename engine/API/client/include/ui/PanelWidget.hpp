#pragma once

#include "Widget.hpp"

namespace TechEngine {
    class PanelWidget;
}

namespace TechEngineAPI {
    class API_DLL PanelWidget : public Widget {
    public:
        explicit PanelWidget(std::shared_ptr<TechEngine::PanelWidget>& widget);

        const glm::vec4& getBackgroundColor();

        void setBackgroundColor(const glm::vec4& color);
    };
}
