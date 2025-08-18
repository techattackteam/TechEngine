#include "Widget.hpp"

#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "core/Logger.hpp"

namespace TechEngine {
    Widget::Widget() {
        setAnchorsFromPreset();
    }

    Widget::Widget(Widget* widget) {
        this->m_name = widget->m_name;
        this->m_category = widget->m_category;
        this->m_description = widget->m_description;
        for (const auto& prop: widget->m_properties) {
            WidgetProperty property;
            property.name = prop.name;
            property.type = prop.type;
            property.defaultValue = prop.defaultValue;
            property.onChange = prop.onChange; // Copy the onChange callback
            this->m_properties.push_back(property);
        }
        for (const auto& childType: widget->m_childrenTypes) {
            this->m_childrenTypes.push_back(childType);
        }
    }

    void Widget::update(float deltaTime) {
        for (const auto& child: m_children) {
            if (child) {
                child->update(deltaTime);
            }
        }
    }

    void Widget::draw(UIRenderer& renderer) {
        for (const auto& child: m_children) {
            if (child) {
                child->draw(renderer);
            }
        }
    }

    void Widget::changeAnchor(AnchorPreset anchorPreset, glm::vec4 parentScreenRect) {
        if (anchorPreset == m_preset) {
            return; // No change needed
        }

        // 1. Store the current absolute screen rect. This is the visual state we must preserve.
        const glm::vec4 oldScreenRect = m_finalScreenRect;

        // 2. Update the internal anchor preset and min/max values
        m_preset = anchorPreset;
        setAnchorsFromPreset();

        // 3. Determine if the NEW preset is a stretching preset
        bool isStretchingX = (m_anchorMax.x - m_anchorMin.x) > 0.001f;
        bool isStretchingY = (m_anchorMax.y - m_anchorMin.y) > 0.001f;

        // 4. Perform the "reverse layout" calculation to find the new offset values

        // --- Calculate new X-axis properties ---
        if (isStretchingX) {
            // The new mode is STRETCH. We need to calculate m_left and m_right.
            float newAnchorMinX_abs = parentScreenRect.x + parentScreenRect.z * m_anchorMin.x;
            float newAnchorMaxX_abs = parentScreenRect.x + parentScreenRect.z * m_anchorMax.x;

            m_left = oldScreenRect.x - newAnchorMinX_abs;
            m_right = newAnchorMaxX_abs - (oldScreenRect.x + oldScreenRect.z);
        } else {
            // The new mode is POINT. We need to calculate m_anchoredPosition.x.
            float newAnchorMinX_abs = parentScreenRect.x + parentScreenRect.z * m_anchorMin.x;

            m_anchoredPosition.x = oldScreenRect.x - newAnchorMinX_abs + (m_size.x * m_pivot.x);
        }

        // --- Calculate new Y-axis properties ---
        if (isStretchingY) {
            // The new mode is STRETCH. We need to calculate m_top and m_bottom.
            float newAnchorMinY_abs = parentScreenRect.y + parentScreenRect.w * m_anchorMin.y;
            float newAnchorMaxY_abs = parentScreenRect.y + parentScreenRect.w * m_anchorMax.y;

            m_top = oldScreenRect.y - newAnchorMinY_abs;
            m_bottom = newAnchorMaxY_abs - (oldScreenRect.y + oldScreenRect.w);
        } else {
            // The new mode is POINT. We need to calculate m_anchoredPosition.y.
            float newAnchorMinY_abs = parentScreenRect.y + parentScreenRect.w * m_anchorMin.y;

            m_anchoredPosition.y = oldScreenRect.y - newAnchorMinY_abs + (m_size.y * m_pivot.y);
        }
    }

    void Widget::setAnchorsFromPreset() {
        switch (m_preset) {
            case AnchorPreset::TopLeft:
                m_anchorMin = {0.0f, 0.0f};
                m_anchorMax = {0.0f, 0.0f};
                break;
            case AnchorPreset::TopCenter:
                m_anchorMin = {0.5f, 0.0f};
                m_anchorMax = {0.5f, 0.0f};
                break;
            case AnchorPreset::TopRight:
                m_anchorMin = {1.0f, 0.0f};
                m_anchorMax = {1.0f, 0.0f};
                break;
            case AnchorPreset::MiddleLeft:
                m_anchorMin = {0.0f, 0.5f};
                m_anchorMax = {0.0f, 0.5f};
                break;
            case AnchorPreset::MiddleCenter:
                m_anchorMin = {0.5f, 0.5f};
                m_anchorMax = {0.5f, 0.5f};
                break;
            case AnchorPreset::MiddleRight:
                m_anchorMin = {1.0f, 0.5f};
                m_anchorMax = {1.0f, 0.5f};
                break;
            case AnchorPreset::BottomLeft:
                m_anchorMin = {0.0f, 1.0f};
                m_anchorMax = {0.0f, 1.0f};
                break;
            case AnchorPreset::BottomCenter:
                m_anchorMin = {0.5f, 1.0f};
                m_anchorMax = {0.5f, 1.0f};
                break;
            case AnchorPreset::BottomRight:
                m_anchorMin = {1.0f, 1.0f};
                m_anchorMax = {1.0f, 1.0f};
                break;
            // Stretch presets
            case AnchorPreset::StretchTop:
                m_anchorMin = {0.0f, 0.0f};
                m_anchorMax = {1.0f, 0.0f};
                break;
            case AnchorPreset::StretchMiddle:
                m_anchorMin = {0.0f, 0.5f};
                m_anchorMax = {1.0f, 0.5f};
                break;
            case AnchorPreset::StretchBottom:
                m_anchorMin = {0.0f, 1.0f};
                m_anchorMax = {1.0f, 1.0f};
                break;
            case AnchorPreset::StretchLeft:
                m_anchorMin = {0.0f, 0.0f};
                m_anchorMax = {0.0f, 1.0f};
                break;
            case AnchorPreset::StretchCenter:
                m_anchorMin = {0.5f, 0.0f};
                m_anchorMax = {0.5f, 1.0f};
                break;
            case AnchorPreset::StretchRight:
                m_anchorMin = {1.0f, 0.0f};
                m_anchorMax = {1.0f, 1.0f};
                break;
            case AnchorPreset::StretchFill:
                m_anchorMin = {0.0f, 0.0f};
                m_anchorMax = {1.0f, 1.0f};
                break;
        }
    }

    void Widget::rename(const std::string& name) {
        m_name = name;
    }

    void Widget::calculateLayout(const glm::vec4& parentScreenRect) {
        // parentScreenRect is {x, y, width, height}

        // 1. Calculate the absolute screen positions of the anchor points within the parent
        glm::vec2 anchorMinPos = {parentScreenRect.x + parentScreenRect.z * m_anchorMin.x, parentScreenRect.y + parentScreenRect.w * m_anchorMin.y};
        glm::vec2 anchorMaxPos = {parentScreenRect.x + parentScreenRect.z * m_anchorMax.x, parentScreenRect.y + parentScreenRect.w * m_anchorMax.y};

        // 2. Determine if we are stretching on each axis
        bool isStretchingX = (m_anchorMax.x - m_anchorMin.x) > 0.001f;
        bool isStretchingY = (m_anchorMax.y - m_anchorMin.y) > 0.001f;

        // 3. Calculate final position and size based on stretch mode

        // --- Handle X-Axis ---
        if (isStretchingX) {
            m_finalScreenRect.x = anchorMinPos.x + m_left;
            float rightEdge = anchorMaxPos.x - m_right;
            m_finalScreenRect.z = rightEdge - m_finalScreenRect.x; // width
        } else {
            // Not stretching, so we anchor to a single point and set a fixed size
            m_finalScreenRect.z = m_size.x; // width
            m_finalScreenRect.x = anchorMinPos.x + m_anchoredPosition.x - (m_size.x * m_pivot.x);
        }

        // --- Handle Y-Axis ---
        if (isStretchingY) {
            m_finalScreenRect.y = anchorMinPos.y + m_top;
            float bottomEdge = anchorMaxPos.y - m_bottom;
            m_finalScreenRect.w = bottomEdge - m_finalScreenRect.y; // height
        } else {
            // Not stretching, so we anchor to a single point and set a fixed size
            m_finalScreenRect.w = m_size.y; // height
            m_finalScreenRect.y = anchorMinPos.y + m_anchoredPosition.y - (m_size.y * m_pivot.y);
        }

        // 4. After calculating our own layout, recursively call layout calculation for all children
        for (const auto& child: m_children) {
            if (child) {
                child->calculateLayout(m_finalScreenRect);
            }
        }
    }

    void Widget::addChild(const std::shared_ptr<Widget>& child, int index) {
        if (!child) {
            TE_LOGGER_ERROR("Widget::addChild: Attempted to add a null child widget.");
            return;
        }
        if (index < 0 || index > m_children.size()
        ) {
            TE_LOGGER_ERROR("Widget::addChild: Index out of bounds. Adding child at the end.");
            index = m_children.size(); // Add to the end if index is invalid
        }
        m_children.insert(m_children.begin() + index, child);
        child->m_parent = shared_from_this();
    }

    void Widget::removeChild(const std::shared_ptr<Widget>& child) {
        auto it = std::ranges::find(m_children, child);
        if (it != m_children.end()) {
            m_children.erase(it, m_children.end());
        }
        child->m_parent = nullptr;
    }

    // Add this method to the Widget class
    glm::vec2 Widget::getAbsoluteOffset() const {
        if (m_parent) {
            return m_parent->getAbsoluteOffset() + m_anchoredPosition;
        }
        return m_anchoredPosition;
    }
}
