#pragma once
#include "Widget.hpp"

namespace TechEngine {
    class InputTextWidget;
}

namespace TechEngineAPI {
    class API_DLL InputTextWidget : public Widget {
    public:
        explicit InputTextWidget(std::shared_ptr<TechEngine::InputTextWidget>& widget);
    };
}
