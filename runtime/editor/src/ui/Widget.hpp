#pragma once
#include <variant>

#include <RmlUi/Core/Element.h>


namespace TechEngine {
    struct WidgetProperty {
        std::string name;
        std::string rcssProperty;
        std::string type;
        std::string defaultValue;

        using ProperTyValue = std::variant<Rml::Vector2f, Rml::Vector3f, std::string, Rml::Colourb, bool, int, float>;
        using OnChangeCallback = std::function<void(const ProperTyValue&)>;
        OnChangeCallback onChange;

    };

    class Widget {
    public:
        Rml::Element* m_rmlElement = nullptr; // Pointer to the RmlUi element this widget represents

        std::string m_name;
        std::string m_category;
        std::string m_description;
        std::string m_rmlSnippet;

        Rml::Vector2f m_position;
        Rml::Vector2f m_size;
        std::vector<WidgetProperty> m_properties;

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

        /*template<typename T>
        void addProperty(const std::string& name,
                         PropertyType type,
                         const T& initialValue,
                         WidgetProperty::OnChangeCallback onChange = nullptr,
                         const std::vector<std::string>& enumOptions = {}
        ) {
            assert(!name.empty() && "Property name cannot be empty");
            assert(type != PropertyType::Enum || !enumOptions.empty() && "Enum properties must have options");
            m_properties.push_back({name, type, initialValue, enumOptions, onChange});
        }*/
    };
}
