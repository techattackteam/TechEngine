#include "Widget.hpp"

namespace TechEngine {
    Widget::Widget() {
        //if (m_rmlElement) {
        /*for (auto& property: m_properties) {
            property.onChange = [property, this](const WidgetProperty::ProperTyValue& value) {
                // Default no-op for properties without a specific onChange handler
                if (property.type == "color") {
                    m_rmlElement->SetProperty(Rml::PropertyId::BackgroundColor, Rml::Property(std::get<Rml::Colourb>(value), Rml::Unit::COLOUR));
                }
            };
        }*/
        //}
        /*addProperty<std::string>("Name", PropertyType::String, m_name, [this](auto& value) {
            m_name = std::get<std::string>(value);
            if (m_rmlElement) {
                m_rmlElement->SetId(m_name);
            }
        });
        addProperty<std::string>("ID", PropertyType::String, std::to_string(m_id), [this](auto& value) {
            // ID is not directly settable in RmlUi, but we can use it for internal tracking
            m_id = std::stoul(std::get<std::string>(value));
        });

        addProperty<std::string>("Classes", PropertyType::String, std::string(""), [this](const auto& value) {
            if (m_rmlElement) {
                m_rmlElement->SetClassNames(std::get<std::string>(value));
            }
        });

        std::vector<std::string> visibilityOptions = {"Visible", "Hidden", "Collapsed"};
        addProperty<int>("Visibility", PropertyType::Enum, 0, [this](auto& value) {
            if (m_rmlElement) {
                int visibility = std::get<int>(value);
                switch (visibility) {
                    case 0: // Visible
                        m_rmlElement->SetProperty("Visibility", "visible");
                        //m_rmlElement->RemoveProperty("display");
                        break;
                    case 1: // Hidden
                        m_rmlElement->SetProperty("visibility", "hidden");
                        //m_rmlElement->RemoveProperty("display");
                        break;
                    case 2: // Collapsed
                        m_rmlElement->SetProperty("display", "none");
                        break;
                }
            }
        }, visibilityOptions);
        std::vector<std::string> alignmentOptions = {"Left", "Center", "Right"};
        addProperty<int>("Alignment", PropertyType::Enum, 0, [this](auto& value) {
            if (m_rmlElement) {
                int alignment = std::get<int>(value);
                switch (alignment) {
                    case 0: // Left
                        if (!m_rmlElement->SetProperty(Rml::PropertyId::AlignItems, Rml::Property("left", Rml::Unit::KEYWORD))) {
                            TE_LOGGER_CRITICAL("Failed to set self alignment to flex-start for element: {}", m_rmlElement->GetId().c_str());
                        }

                        break;
                    case 1: // Center
                        if (m_rmlElement->SetProperty(Rml::PropertyId::AlignItems, Rml::Property("center", Rml::Unit::KEYWORD))) {
                            TE_LOGGER_CRITICAL("Failed to set self alignment to center for element: {}", m_rmlElement->GetId().c_str());
                        }
                        break;
                    case 2: // Right
                        m_rmlElement->SetProperty("self alignment", "flex-end");
                        break;
                }
            }
        }, alignmentOptions);

        addProperty<Rml::Vector2f>("Position", PropertyType::Vector2f, {10, 10}, [this](auto& value) {
            auto position = std::get<Rml::Vector2f>(value);
            if (m_rmlElement) {
                m_rmlElement->SetProperty(Rml::PropertyId::Left, Rml::Property(position.x, Rml::Unit::PX));
                m_rmlElement->SetProperty(Rml::PropertyId::Top, Rml::Property(position.y, Rml::Unit::PX));
            }
        });
        addProperty<Rml::Vector2f>("Size", PropertyType::Vector2f, {100, 100}, [this](auto& value) {
            auto position = std::get<Rml::Vector2f>(value);
            if (m_rmlElement) {
                m_rmlElement->SetProperty(Rml::PropertyId::Width, Rml::Property(position.x, Rml::Unit::PX));
                m_rmlElement->SetProperty(Rml::PropertyId::Height, Rml::Property(position.y, Rml::Unit::PX));
            }
        });
        addProperty<int>("Z-Index", PropertyType::Int, 0, [this](auto& value) {
            if (m_rmlElement) {
                m_rmlElement->SetProperty(Rml::PropertyId::ZIndex, Rml::Property(std::get<int>(value), Rml::Unit::NUMBER));
            }
        });*/
    }

    Widget::Widget(Widget* widget) {
        this->m_name = widget->m_name;
        this->m_category = widget->m_category;
        this->m_description = widget->m_description;
        //this->m_rmlSnippet = widget->m_rmlSnippet;
        for (const auto& prop: widget->m_properties) {
            WidgetProperty property;
            property.name = prop.name;
            property.rcssProperty = prop.rcssProperty;
            property.type = prop.type;
            property.defaultValue = prop.defaultValue;
            property.onChange = prop.onChange; // Copy the onChange callback
            this->m_properties.push_back(property);
        }
        // Deep Copy children
        for (const auto& childType: widget->m_childrenTypes) {
            this->m_childrenTypes.push_back(childType);
        }
        this->m_rmlElement = nullptr;
    }
}
