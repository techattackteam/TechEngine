#include "client/include/ui/ContainerWidget.hpp"

#include <ui/ContainerWidget.hpp>


namespace TechEngineAPI {
    ContainerWidget::ContainerWidget(std::shared_ptr<TechEngine::ContainerWidget>& widget) : Widget(std::dynamic_pointer_cast<TechEngine::ContainerWidget>(widget)) {
    }
}
