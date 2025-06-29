#pragma once
#include <functional>
#include <string>
#include <variant>
#include <RmlUi/Core/Types.h>

namespace TechEngine {
    enum class PropertyType {
        Vector2f,
        Vector3f,
        String,
        Color,
        Bool,
        Int,
        Float,
        Enum,
    };

    using ProperTyValue = std::variant<Rml::Vector2f, Rml::Vector3f, std::string, Rml::Colourb, bool, int, float>;

    struct WidgetProperty {
        std::string name;
        PropertyType type;
        ProperTyValue value;
        std::vector<std::string> enumOptions;

        using OnChangeCallback = std::function<void(const ProperTyValue&)>;
        OnChangeCallback onChange;
    };
}
