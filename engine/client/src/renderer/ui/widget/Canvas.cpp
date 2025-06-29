#include "Canvas.hpp"


namespace TechEngine {
    Canvas::Canvas(const std::string& name) : Widget(name) {
        addProperty<Rml::Colourb>("Background Color", PropertyType::Color, Rml::Colourb(255, 255, 255), [this](const auto& value) {
            if (m_rmlElement) {
                m_rmlElement->SetProperty(Rml::PropertyId::BackgroundColor, Rml::Property(std::get<Rml::Colourb>(value), Rml::Unit::COLOUR));
                
            }
        });
    }

    const char* Canvas::getRmlTag() const {
        return "div";
    }
}
