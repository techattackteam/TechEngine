#pragma once

#include "core/ExportDLL.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>

namespace TechEngine {
    struct CLIENT_DLL WidgetProperty {
        std::string name;
        std::string type;
        std::string defaultValue;

        using OnChangeCallback = std::function<void(const std::string&)>;
        OnChangeCallback onChange;
    };

    class CLIENT_DLL Widget {
    public:
        enum class AnchorPreset {
            // Top Left Corner
            TopLeft,
            TopCenter,
            TopRight,

            // Middle Row
            MiddleLeft,
            MiddleCenter,
            MiddleRight,

            // Bottom Row
            BottomLeft,
            BottomCenter,
            BottomRight,

            // Stretching
            StretchTop,
            StretchMiddle,
            StretchBottom,
            StretchLeft,
            StretchCenter,
            StretchRight,

            // Full Stretch
            StretchFill
        };


        std::string m_name;
        std::string m_category;
        std::string m_description;

        // --- State controlled by the UI Inspector ---
        AnchorPreset m_preset = AnchorPreset::MiddleCenter;

        // AnchorMin/Max range from (0,0) [bottom-left] to (1,1) [top-right] in RmlUi
        glm::vec2 m_anchorMin = {0.0f, 0.0f};
        glm::vec2 m_anchorMax = {0.0f, 0.0f};

        // Pivot also ranges from (0,0) to (1,1)
        glm::vec2 m_pivot = {0.5f, 0.5f};

        // This field is used when NOT stretching (e.g. AnchorPreset::TopLeft)
        glm::vec2 m_size = {100.0f, 100.0f};

        // This field's meaning changes based on the anchor preset
        glm::vec2 m_anchoredPosition = {0.0f, 0.0f};

        // These fields are used when stretching
        float m_left = 0.0f;
        float m_right = 0.0f;
        float m_top = 0.0f;
        float m_bottom = 0.0f;
        float m_rotationZ = 0.0f;

        //glm::vec2 m_position;
        //glm::vec2 m_size;
        std::vector<WidgetProperty> m_properties;
        std::vector<std::string> m_childrenTypes; // Types of children this widget can have, e.g., "Button", "Text", etc.
        std::vector<std::shared_ptr<Widget>> m_children; // Children widgets

    public:
        explicit Widget();

        // Copy constructor for Widget, useful for creating a new widget based on an existing one
        Widget(Widget* widget);

        virtual ~Widget() = default;

        void setAnchorsFromPreset();

        void rename(const std::string& name);

        void applyStyles(Widget* element, Widget* parent) const;

        const std::string& getName() const {
            return m_name;
        }

        virtual const std::vector<WidgetProperty>& getProperties() const {
            return m_properties;
        }

        virtual std::vector<WidgetProperty>& getProperties() {
            return m_properties;
        }
    };
}
