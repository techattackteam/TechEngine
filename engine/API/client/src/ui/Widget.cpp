#include "client/include/ui/Widget.hpp"
#include "ui/Widget.hpp"

namespace TechEngineAPI {
    Widget::Widget(std::shared_ptr<TechEngine::Widget>& widget) : m_widget(widget.get()) {
    }

    const std::string& Widget::getName() {
        return m_widget->getName();
    }
}
