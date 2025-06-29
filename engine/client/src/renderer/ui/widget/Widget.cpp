#include "Widget.hpp"

#include <utility>

namespace TechEngine {
    Widget::Widget(std::string name) : m_name(std::move(name)) {
        static uint32_t next_id = 1;
        m_id = next_id++;

        addProperty<std::string>("Name", PropertyType::String, m_name, [this](auto& value) {
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
                        m_rmlElement->SetProperty("self alignment", "flex-start");
                        break;
                    case 1: // Center
                        m_rmlElement->SetProperty("self alignment", "center");
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
        });
    }
}
