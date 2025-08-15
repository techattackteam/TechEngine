#include "Widget.hpp"

#include "core/Logger.hpp"

namespace TechEngine {
    Widget::Widget() {
        setAnchorsFromPreset();
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

    void Widget::applyStyles(Widget* element, Widget* parent) const {
        /*if (!element) return;

        // 1. Enforce the parent's positioning context
        if (parent /*&& parent->GetProperty("position")->Get<int>() == Rml::Style::Position::Static#1#) {
            //parent->SetProperty("position", "relative");
        }

        // 2. Set the element's own pivot
        element->SetProperty("transform-origin", std::to_string(m_pivot.x * 100) + "% " + std::to_string(m_pivot.y * 100) + "%");

        // 3. Check if we are in a "stretch" mode or a "point" mode
        bool isStretchingX = m_anchorMax.x - m_anchorMin.x > 0.001f;
        bool isStretchingY = m_anchorMax.y - m_anchorMin.y > 0.001f;

        // We must be absolutely positioned to use anchors
        element->SetProperty("position", "absolute");


        if (std::abs(m_rotationZ) > 0.01f) {
            element->SetProperty("transform", "rotate(30deg)");
            auto p = Rml::Transform::MakeProperty({Rml::Transforms::Rotate2D{10.f}});
            element->SetProperty(Rml::PropertyId::Transform, p);
        }
        // You can add scaling/skew here later if needed


        // --- Handle X-Axis ---
        if (isStretchingX) {
            element->SetProperty("left", std::to_string(m_anchorMin.x * 100) + "%");
            element->SetProperty("right", std::to_string((1.0f - m_anchorMax.x) * 100) + "%");
            element->SetProperty("width", "auto");
            // In this mode, `left` and `right` from the component act as margin/padding
            element->SetProperty("margin-left", std::to_string(this->m_left) + "px");
            element->SetProperty("margin-right", std::to_string(this->m_right) + "px");
        } else {
            // Not stretching, so we anchor to a single point and set a fixed size
            element->SetProperty("left", std::to_string(m_anchorMin.x * 100) + "%");
            element->SetProperty("width", std::to_string(m_size.x) + "px");
            element->SetProperty("margin-left", std::to_string(m_anchoredPosition.x - (m_size.x * m_pivot.x)) + "px");
            TE_LOGGER_INFO("Setting X position to: {0}%", std::to_string(m_anchorMin.x * 100));
            TE_LOGGER_INFO("Setting X size to: {0}px", std::to_string(m_size.x));
            TE_LOGGER_INFO("Setting X margin to: {0}px", std::to_string(m_anchoredPosition.x - (m_size.x * m_pivot.x)));
        }

        // --- Handle Y-Axis (similar logic) ---
        if (isStretchingY) {
            element->SetProperty("top", std::to_string(m_anchorMin.y * 100) + "%");
            element->SetProperty("bottom", std::to_string((1.0f - m_anchorMax.y) * 100) + "%");
            element->SetProperty("height", "auto");
            element->SetProperty("margin-top", std::to_string(this->m_top) + "px");
            element->SetProperty("margin-bottom", std::to_string(this->m_bottom) + "px");
        } else {
            // Current non–stretch Y–axis code snippet
            element->SetProperty("top", std::to_string(m_anchorMin.y * 100) + "%");
            element->SetProperty("height", std::to_string(m_size.y) + "px");
            element->SetProperty("margin-top", std::to_string(m_anchoredPosition.y - (m_size.y * m_pivot.y)) + "px");
            TE_LOGGER_INFO("Setting Y position to: {0}%", std::to_string(m_anchorMin.y * 100));
            TE_LOGGER_INFO("Setting Y size to: {0}px", std::to_string(m_size.y));
            TE_LOGGER_INFO("Setting Y margin to: {0}px", std::to_string(m_anchoredPosition.y - (m_size.y * m_pivot.y)));
        }*/
    }
}
