#include "Widget.hpp"

#include "WidgetsRegistry.hpp"
#include "core/Logger.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "events/ui/MouseLeftWidgetRectEvent.hpp"
#include "events/ui/MouseEnteredWidgetRectEvent.hpp"

namespace TechEngine {
    Widget::Widget() {
        setAnchorsFromPreset();
    }

    Widget::Widget(const std::string& name) : m_name(name) {
        setAnchorsFromPreset();
    }

    Widget::Widget(Widget* widget) {
        this->m_name = widget->m_name;
        this->m_category = widget->m_category;
        this->m_description = widget->m_description;

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
        //for (const auto& child: m_children) {
        //    if (child) {
        //        child->draw(renderer);
        //    }
        //}
    }

    void Widget::changeAnchor(AnchorPreset anchorPreset, const glm::vec4& parentScreenRect, float dpiScale) {
        if (anchorPreset == m_preset || dpiScale <= 0.0f) {
            return; // No change needed or invalid scale
        }

        // 1. Store the current absolute screen rect. This is the visual state we must preserve.
        const glm::vec4 oldScreenRect = m_finalScreenRect;

        // Store the old preset's stretching behavior
        bool wasStretchingX = (m_anchorMax.x - m_anchorMin.x) > 0.001f;
        bool wasStretchingY = (m_anchorMax.y - m_anchorMin.y) > 0.001f;

        // 2. Update the internal anchor preset and min/max values to the NEW preset
        m_preset = anchorPreset;
        setAnchorsFromPreset();

        // 3. Determine if the NEW preset is a stretching preset
        bool isStretchingX = (m_anchorMax.x - m_anchorMin.x) > 0.001f;
        bool isStretchingY = (m_anchorMax.y - m_anchorMin.y) > 0.001f;

        // 4. Perform the "reverse layout" calculation to find the new unscaled offset values

        // --- Calculate new X-axis properties ---
        if (isStretchingX) {
            // The new mode is STRETCH. We need to calculate m_left and m_right.
            float newAnchorMinX_abs = parentScreenRect.x + parentScreenRect.z * m_anchorMin.x;
            float newAnchorMaxX_abs = parentScreenRect.x + parentScreenRect.z * m_anchorMax.x;

            float scaledLeft = oldScreenRect.x - newAnchorMinX_abs;
            float scaledRight = newAnchorMaxX_abs - (oldScreenRect.x + oldScreenRect.z);

            // ** THE FIX **: Un-scale the calculated values before storing them.
            m_left = scaledLeft / dpiScale;
            m_right = scaledRight / dpiScale;
        } else {
            // The new mode is POINT. We need to calculate m_anchoredPosition.x.
            // If we just switched from stretch, we must also update m_size.
            if (wasStretchingX) {
                m_size.x = oldScreenRect.z / dpiScale;
            }

            float newAnchorMinX_abs = parentScreenRect.x + parentScreenRect.z * m_anchorMin.x;

            // Use the new m_size (which might have just been updated) to get the correct scaled size.
            float scaledPivotOffsetX = (m_size.x * dpiScale) * m_pivot.x;
            float scaledAnchoredPosX = oldScreenRect.x - newAnchorMinX_abs + scaledPivotOffsetX;

            m_anchoredPosition.x = scaledAnchoredPosX / dpiScale;
        }

        // --- Calculate new Y-axis properties ---
        if (isStretchingY) {
            // The new mode is STRETCH. We need to calculate m_top and m_bottom.
            float newAnchorMinY_abs = parentScreenRect.y + parentScreenRect.w * m_anchorMin.y;
            float newAnchorMaxY_abs = parentScreenRect.y + parentScreenRect.w * m_anchorMax.y;

            float scaledTop = oldScreenRect.y - newAnchorMinY_abs;
            float scaledBottom = newAnchorMaxY_abs - (oldScreenRect.y + oldScreenRect.w);

            m_top = scaledTop / dpiScale;
            m_bottom = scaledBottom / dpiScale;
        } else {
            // The new mode is POINT. We need to calculate m_anchoredPosition.y.
            // If we just switched from stretch, we must also update m_size.
            if (wasStretchingY) {
                m_size.y = oldScreenRect.w / dpiScale;
            }

            float newAnchorMinY_abs = parentScreenRect.y + parentScreenRect.w * m_anchorMin.y;

            float scaledPivotOffsetY = (m_size.y * dpiScale) * m_pivot.y;
            float scaledAnchoredPosY = oldScreenRect.y - newAnchorMinY_abs + scaledPivotOffsetY;

            m_anchoredPosition.y = scaledAnchoredPosY / dpiScale;
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

    std::string Widget::getPropertyType(const std::string& propertyName) const {
        if (m_properties.find(propertyName) != m_properties.end()) {
            return m_properties.at(propertyName);
        } else {
            TE_LOGGER_ERROR("Widget::getPropertyType: Property '{0}' not found in widget '{1}'.", propertyName.c_str(), m_name.c_str());
        }
    }

    void Widget::serialize(YAML::Emitter& out) const {
        out << YAML::Key << "Type" << YAML::Value << m_type;
        out << YAML::Key << "Name" << YAML::Value << m_name;
        out << YAML::Key << "Category" << YAML::Value << m_category;
        out << YAML::Key << "Description" << YAML::Value << m_description;
        out << YAML::Key << "Preset" << YAML::Value << static_cast<int>(m_preset);
        out << YAML::Key << "AnchorMin" << YAML::Value << YAML::Flow << YAML::BeginSeq << m_anchorMin.x << m_anchorMin.y << YAML::EndSeq;
        out << YAML::Key << "AnchorMax" << YAML::Value << YAML::Flow << YAML::BeginSeq << m_anchorMax.x << m_anchorMax.y << YAML::EndSeq;
        out << YAML::Key << "Pivot" << YAML::Value << YAML::Flow << YAML::BeginSeq << m_pivot.x << m_pivot.y << YAML::EndSeq;
        out << YAML::Key << "Size" << YAML::Value << YAML::Flow << YAML::BeginSeq << m_size.x << m_size.y << YAML::EndSeq;
        out << YAML::Key << "AnchoredPosition" << YAML::Value << YAML::Flow << YAML::BeginSeq << m_anchoredPosition.x << m_anchoredPosition.y << YAML::EndSeq;
        out << YAML::Key << "Left" << YAML::Value << m_left;
        out << YAML::Key << "Right" << YAML::Value << m_right;
        out << YAML::Key << "Top" << YAML::Value << m_top;
        out << YAML::Key << "Bottom" << YAML::Value << m_bottom;
        out << YAML::Key << "RotationZ" << YAML::Value << m_rotationZ;
        out << YAML::Key << "Children" << YAML::Value << YAML::BeginSeq;
        for (const auto& child: m_children) {
            out << YAML::BeginMap;
            if (child) {
                child->serialize(out);
            }
            out << YAML::EndMap;
        }
        out << YAML::EndSeq;
    }

    void Widget::deserialize(const YAML::Node& node, WidgetsRegistry& registry) {
        if (node["Type"].IsDefined()) {
            m_type = node["Type"].as<std::string>();
        }
        if (node["Name"]) {
            m_name = node["Name"].as<std::string>();
        }
        if (node["Category"]) {
            m_category = node["Category"].as<std::string>();
        }
        if (node["Description"]) {
            m_description = node["Description"].as<std::string>();
        }
        if (node["Preset"]) {
            m_preset = static_cast<AnchorPreset>(node["Preset"].as<int>());
        }
        if (node["AnchorMin"]) {
            m_anchorMin = node["AnchorMin"].as<glm::vec2>();
        }
        if (node["AnchorMax"]) {
            m_anchorMax = node["AnchorMax"].as<glm::vec2>();
        }
        if (node["Pivot"]) {
            m_pivot = node["Pivot"].as<glm::vec2>();
        }
        if (node["Size"]) {
            m_size = node["Size"].as<glm::vec2>();
        }
        if (node["AnchoredPosition"]) {
            m_anchoredPosition = node["AnchoredPosition"].as<glm::vec2>();
        }
        if (node["Left"]) {
            m_left = node["Left"].as<float>();
        }
        if (node["Right"]) {
            m_right = node["Right"].as<float>();
        }
        if (node["Top"]) {
            m_top = node["Top"].as<float>();
        }
        if (node["Bottom"]) {
            m_bottom = node["Bottom"].as<float>();
        }
        if (node["RotationZ"]) {
            m_rotationZ = node["RotationZ"].as<float>();
        }
        if (node["Children"] && node["Children"].IsSequence()) {
            int i = 0;
            for (const YAML::Node& childNode: node["Children"]) {
                if (childNode["Type"] && childNode["Name"]) {
                    std::string childType = childNode["Type"].as<std::string>();
                    std::string childName = childNode["Name"].as<std::string>();
                    auto childWidget = registry.createWidget(shared_from_this(), childType, childName, false);
                    if (childWidget) {
                        childWidget->deserialize(childNode, registry);
                        registry.performWidgetMove(childWidget, shared_from_this(), i);
                        i++;
                    } else {
                        TE_LOGGER_ERROR("Widget::deserialize: Failed to create child widget of type '{0}' with name '{1}'.", childType.c_str(), childName.c_str());
                    }
                } else {
                    TE_LOGGER_ERROR("Widget::deserialize: Child widget node is missing 'Type' or 'Name' fields.");
                }
            }
        }
    }

    void Widget::onMouseEnteredRect(EventDispatcher& eventDispatcher) {
        m_mouseHovering = true;
        eventDispatcher.dispatch<MouseEnteredWidgetRectEvent>(shared_from_this());
    }

    void Widget::onMouseLeftRect(EventDispatcher& eventDispatcher) {
        m_mouseHovering = false;
        eventDispatcher.dispatch<MouseLeftWidgetRectEvent>(shared_from_this());
    }

    void Widget::calculateLayout(const glm::vec4& parentScreenRect, float dpiScale) {
        // parentScreenRect is {x, y, width, height}

        // --- Scale all pixel-dependent properties FIRST ---
        glm::vec2 scaledSize = m_size * dpiScale;
        glm::vec2 scaledAnchoredPosition = m_anchoredPosition * dpiScale;
        float scaledLeft = m_left * dpiScale;
        float scaledRight = m_right * dpiScale;
        float scaledTop = m_top * dpiScale;
        float scaledBottom = m_bottom * dpiScale;

        // 1. Calculate the absolute screen positions of the anchor points within the parent
        glm::vec2 anchorMinPos = {parentScreenRect.x + parentScreenRect.z * m_anchorMin.x, parentScreenRect.y + parentScreenRect.w * m_anchorMin.y};
        glm::vec2 anchorMaxPos = {parentScreenRect.x + parentScreenRect.z * m_anchorMax.x, parentScreenRect.y + parentScreenRect.w * m_anchorMax.y};

        // 2. Determine if we are stretching on each axis
        bool isStretchingX = (m_anchorMax.x - m_anchorMin.x) > 0.001f;
        bool isStretchingY = (m_anchorMax.y - m_anchorMin.y) > 0.001f;

        // 3. Calculate final position and size based on stretch mode
        // --- Handle X-Axis ---
        if (isStretchingX) {
            m_finalScreenRect.x = anchorMinPos.x + scaledLeft;
            float rightEdge = anchorMaxPos.x - scaledRight;
            m_finalScreenRect.z = rightEdge - m_finalScreenRect.x; // width
        } else {
            // Not stretching, so we anchor to a single point and set a fixed size
            m_finalScreenRect.z = scaledSize.x; // width
            m_finalScreenRect.x = anchorMinPos.x + scaledAnchoredPosition.x - (scaledSize.x * m_pivot.x);
        }

        // --- Handle Y-Axis ---
        if (isStretchingY) {
            m_finalScreenRect.y = anchorMinPos.y + scaledTop;
            float bottomEdge = anchorMaxPos.y - scaledBottom;
            m_finalScreenRect.w = bottomEdge - m_finalScreenRect.y; // height
        } else {
            // Not stretching, so we anchor to a single point and set a fixed size
            m_finalScreenRect.w = scaledSize.y; // height
            m_finalScreenRect.y = anchorMinPos.y + scaledAnchoredPosition.y - (scaledSize.y * m_pivot.y);
        }

        // 4. After calculating our own layout, recursively call layout calculation for all children
        for (const auto& child: m_children) {
            if (child) {
                child->calculateLayout(m_finalScreenRect, dpiScale);
            }
        }
    }


    void Widget::addChild(const std::shared_ptr<Widget>& child, int index) {
        if (!child) {
            TE_LOGGER_ERROR("Widget::addChild: Attempted to add a null child widget.");
            return;
        }
        if (index < 0 || index > m_children.size()) {
            TE_LOGGER_ERROR("Widget::addChild: Index out of bounds. Adding child at the end.");
            index = m_children.size(); // Add to the end if index is invalid
        }
        m_children.insert(m_children.begin() + index, child);
        child->m_parent = shared_from_this();
    }

    void Widget::removeChild(const std::shared_ptr<Widget>& child) {
        auto it = std::find(m_children.begin(), m_children.end(), child);
        if (it != m_children.end()) {
            m_children.erase(it);
            child->m_parent = nullptr;
        } else {
            TE_LOGGER_ERROR("Widget::removeChild: Child widget not found.");
        }
    }


    glm::vec2 Widget::getAbsoluteOffset() {
        if (m_parent) {
            return m_parent->getAbsoluteOffset() + m_anchoredPosition;
        }
        return m_finalScreenRect;
    }

    // Add this method to the Widget class
    glm::vec2 Widget::getAbsoluteOffset() const {
        if (m_parent) {
            return m_parent->getAbsoluteOffset() + m_anchoredPosition;
        }
        return m_anchoredPosition;
    }

    glm::vec4 Widget::getFinalScreenRect() {
        return m_finalScreenRect;
    }

    glm::vec4 Widget::getFinalScreenRect() const {
        return m_finalScreenRect;
    }
}
