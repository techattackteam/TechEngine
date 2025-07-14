#pragma once

#include <RmlUi/Core/Element.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace TechEngine {
    struct WidgetProperty {
        std::string name;
        std::string rcssProperty;
        std::string type;
        std::string defaultValue;

        using OnChangeCallback = std::function<void(const std::string&)>;
        OnChangeCallback onChange;
    };

    class Widget {
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

        Rml::Element* m_rmlElement = nullptr; // Pointer to the RmlUi element this widget represents

        std::string m_name;
        std::string m_category;
        std::string m_description;

        // --- State controlled by the UI Inspector ---
        AnchorPreset m_preset = AnchorPreset::MiddleCenter;

        // AnchorMin/Max range from (0,0) [bottom-left] to (1,1) [top-right] in RmlUi
        Rml::Vector2f m_anchorMin = {0.0f, 0.0f};
        Rml::Vector2f m_anchorMax = {0.0f, 0.0f};

        // Pivot also ranges from (0,0) to (1,1)
        Rml::Vector2f m_pivot = {0.5f, 0.5f};

        // This field is used when NOT stretching (e.g. AnchorPreset::TopLeft)
        Rml::Vector2f m_size = {100.0f, 100.0f};

        // This field's meaning changes based on the anchor preset
        Rml::Vector2f m_anchoredPosition = {0.0f, 0.0f};

        // These fields are used when stretching
        float m_left = 0.0f;
        float m_right = 0.0f;
        float m_top = 0.0f;
        float m_bottom = 0.0f;

        //Rml::Vector2f m_position;
        //Rml::Vector2f m_size;
        std::vector<WidgetProperty> m_properties;
        std::vector<std::string> m_childrenTypes; // Types of children this widget can have, e.g., "Button", "Text", etc.
        std::vector<std::shared_ptr<Widget>> m_children; // Children widgets

    public:
        explicit Widget();

        // Copy constructor for Widget, useful for creating a new widget based on an existing one
        Widget(Widget* widget);

        virtual ~Widget() = default;

        void setAnchorsFromPreset();

        void setRmlElement(Rml::Element* element) {
            m_rmlElement = element;
        }

        void applyStyles(Rml::Element* element, Rml::Element* parent);

        Rml::Element* getRmlElement() const {
            return m_rmlElement;
        }

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
