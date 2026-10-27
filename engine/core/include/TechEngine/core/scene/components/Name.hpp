#pragma once

#include <string>
#include <string_view>

namespace TechEngine {
    struct Name {
        static constexpr std::string_view tag = "TechEngine::Name";

        std::string value = "Entity";

        template<typename Archive>
        friend void visit(Archive& archive, Name& name) {
            archive.field("value", name.value);
        }
    };
}
