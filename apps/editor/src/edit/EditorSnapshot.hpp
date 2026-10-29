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

        bool findRow(Entity entity, std::size_t& out) const;

        Entity getParentOfRow(std::size_t row) const;

        std::size_t getSiblingIndexOfRow(std::size_t row) const;

        // gap counts parent's children as drawn, dragged included; Scene counts a position among
        // the other children, so a move down inside the same parent lands one gap earlier.
        bool resolveDrop(Entity dragged, Entity parent, std::size_t gap, std::size_t& position) const;
    };
}
