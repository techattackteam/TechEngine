#pragma once
#include <cassert>

#include "core/ExportDLL.hpp"
#include "WidgetProperty.hpp"

#include <RmlUi/Core/Element.h>

namespace TechEngine {
    class CLIENT_DLL Widget {
    public:
        Rml::Element* m_rmlElement = nullptr; // Pointer to the RmlUi element this widget represents

        std::string m_name;
        uint32_t m_id = 0;
        std::vector<WidgetProperty> m_properties;

    public:
        explicit Widget(std::string name);

        virtual ~Widget() = default;

        [[nodiscard]] virtual const char* getRmlTag() const = 0;

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

        template<typename T>
        void addProperty(const std::string& name,
                         PropertyType type,
                         const T& initialValue,
                         WidgetProperty::OnChangeCallback onChange = nullptr,
                         const std::vector<std::string>& enumOptions = {}
        ) {
            assert(!name.empty() && "Property name cannot be empty");
            assert(type != PropertyType::Enum || !enumOptions.empty() && "Enum properties must have options");
            m_properties.push_back({name, type, initialValue, enumOptions, onChange});
        }
    };
}
