#include "client/include/ui/InputTextWidget.hpp"
#include "ui/InputTextWidget.hpp"

namespace TechEngineAPI {
    InputTextWidget::InputTextWidget(std::shared_ptr<TechEngine::InputTextWidget>& widget) : Widget(std::dynamic_pointer_cast<TechEngine::InputTextWidget>(widget)) {
    }
}
