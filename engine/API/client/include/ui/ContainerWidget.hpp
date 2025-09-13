#pragma once

#include "Widget.hpp"

namespace TechEngine {
    class ContainerWidget;
}

namespace TechEngineAPI {
    class API_DLL ContainerWidget : public Widget {
    public:
        explicit ContainerWidget(std::shared_ptr<TechEngine::ContainerWidget>& widget);
    };
}
