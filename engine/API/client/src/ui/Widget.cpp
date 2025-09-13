#include "client/include/ui/Widget.hpp"
#include "ui/Widget.hpp"

#include "core/Logger.hpp"

namespace TechEngineAPI {
    Widget::Widget(std::shared_ptr<TechEngine::Widget>& widget) : m_internalWidget(widget.get()) {
    }

    std::shared_ptr<Widget> Widget::getParent() {
        return m_parent;
    }

    std::shared_ptr<Widget> Widget::getChild(const std::string& name) {
        for (auto& child: m_children) {
            if (child->getName() == name) {
                return child;
            }
        }
        TE_LOGGER_ERROR("Child of {0} widget with name '{1}' not found.", getName(), name.c_str());
        return nullptr;
    }

    const std::string& Widget::getName() {
        return m_internalWidget->getName();
    }

    const glm::vec2& Widget::getSize() {
        if (m_internalWidget->m_preset == TechEngine::Widget::AnchorPreset::StretchBottom ||
            m_internalWidget->m_preset == TechEngine::Widget::AnchorPreset::StretchCenter ||
            m_internalWidget->m_preset == TechEngine::Widget::AnchorPreset::StretchFill ||
            m_internalWidget->m_preset == TechEngine::Widget::AnchorPreset::StretchLeft ||
            m_internalWidget->m_preset == TechEngine::Widget::AnchorPreset::StretchMiddle ||
            m_internalWidget->m_preset == TechEngine::Widget::AnchorPreset::StretchRight ||
            m_internalWidget->m_preset == TechEngine::Widget::AnchorPreset::StretchTop) {
            TE_LOGGER_WARN("Widget '{0}' has a stretching anchor preset, size is not be reliable.", getName());
        }
        return m_internalWidget->m_size;
    }

    const void Widget::setSize(const glm::vec2& size) {
        m_internalWidget->m_size = size;
        m_internalWidget->m_isDirty = true;
    }

    const glm::vec2& Widget::getPosition() {
        return m_internalWidget->m_anchoredPosition;
    }

    const void Widget::setPosition(const glm::vec2& position) {
        // Note: This sets the anchored position, which is relative to the anchor point.
        m_internalWidget->m_anchoredPosition = position;
        m_internalWidget->m_isDirty = true;
    }
}
