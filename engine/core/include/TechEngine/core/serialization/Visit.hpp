#pragma once

#include <concepts>

namespace TechEngine {
    template<typename T, typename Archive>
    concept Visitable = requires(Archive& archive, T& value) {
        { visit(archive, value) } -> std::same_as<void>;
    };
}
