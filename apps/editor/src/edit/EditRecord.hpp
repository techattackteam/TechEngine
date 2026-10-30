#pragma once

#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <edit/FieldValue.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace TechEngine {
    struct FieldEditRecord {
        Entity entity;
        ComponentTypeId component;
        std::uint32_t field = 0;
        FieldValue oldValue;
    };

    struct RenameRecord {
        Entity entity;
        std::string oldName;
    };

    struct ComponentAddedRecord {
        Entity entity;
        ComponentTypeId component;
    };

    struct ComponentRemovedRecord {
        Entity entity;
        ComponentTypeId component;
        // The component's writePayload keeps an Entity field's index without its generation, and an
        // AssetRef's id without its handle.
        std::vector<std::byte> payload;
    };

    using EditRecord = std::variant<FieldEditRecord, RenameRecord, ComponentAddedRecord, ComponentRemovedRecord>;
}
