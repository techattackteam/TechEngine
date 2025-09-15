#pragma once

#include "Widget.hpp"

namespace TechEngine {
    class InteractableWidget;
}

namespace TechEngineAPI {
    class API_DLL InteractableWidget : public Widget {
    public:
        explicit InteractableWidget(std::shared_ptr<TechEngine::InteractableWidget>& widget);

        bool isClickable() const;

        void setClickable(bool clickable);
    };
}
