#pragma once
#include <RmlUi/Core/Element.h>


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
        Rml::Element* m_rmlElement = nullptr; // Pointer to the RmlUi element this widget represents

        std::string m_name;
        std::string m_category;
        std::string m_description;

        Rml::Vector2f m_position;
        Rml::Vector2f m_size;
        std::vector<WidgetProperty> m_properties;
        std::vector<std::string> m_childrenTypes; // Types of children this widget can have, e.g., "Button", "Text", etc.
        std::vector<std::shared_ptr<Widget>> m_children; // Children widgets
    public:
        explicit Widget();

        // Copy constructor for Widget, useful for creating a new widget based on an existing one
        Widget(Widget* widget);

        virtual ~Widget() = default;

        //[[nodiscard]] virtual const char* getRmlTag() const = 0;

        void setRmlElement(Rml::Element* element) {
            m_rmlElement = element;
        }

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
