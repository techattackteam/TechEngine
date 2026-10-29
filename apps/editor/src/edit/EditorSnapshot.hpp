#pragma once

#include <TechEngine/core/scene/Entity.hpp>

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace TechEngine {
    struct HierarchyRow {
        Entity entity;
        std::uint32_t depth = 0;
        std::string name;
        std::size_t childCount = 0;
    };

    struct EditorSnapshot {
        // Depth-first: a row's subtree is the run of rows after it with a greater depth. Roots come
        // in storage order, which changes when an entity is destroyed or moves to another archetype.
        std::vector<HierarchyRow> hierarchy;
        Entity selected;
        std::uint64_t editRevision = 0;
        std::uint64_t savedRevision = 0;

        bool isInSubtree(Entity entity, Entity root) const;
    };
}
