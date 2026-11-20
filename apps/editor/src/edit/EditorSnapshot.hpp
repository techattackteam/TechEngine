#pragma once

#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <assets/AssetIndex.hpp>
#include <edit/FieldValue.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace TechEngine {
    struct HierarchyRow {
        Entity entity;
        std::uint32_t depth = 0;
        std::string name;
        std::size_t childCount = 0;
    };

    struct FieldRow {
        std::uint32_t index = 0;
        std::string name;
        FieldKind kind = FieldKind::Bool;
        FieldValue value;
        // Set only on an Asset row.
        StringId assetType;
        bool mixed = false;
    };

    struct ComponentRow {
        ComponentTypeId type;
        std::string tag;
        bool removable = false;
        // Set when a selected entity's walk gives another number of rows than the active one's. The rows' own mixed
        // flags compare only the rows both have, so they miss it.
        bool fieldCountMixed = false;
        std::vector<FieldRow> fields;
    };

    struct TransformRow {
        Vec3 position = Vec3(0.0f);
        Quat rotation = Quat(1.0f, 0.0f, 0.0f, 0.0f);
        Vec3 scale = Vec3(1.0f);
        std::array<bool, 3> positionMixed{};
        std::array<bool, 3> rotationAxesMixed{};
        std::array<bool, 3> scaleMixed{};
    };

    struct ComponentChoice {
        ComponentTypeId type;
        std::string tag;
    };

    struct EditorSnapshot {
        std::vector<HierarchyRow> hierarchy;
        std::vector<Entity> selection;
        Entity active;
        std::uint64_t selectionSequence = 0;
        std::vector<ComponentRow> components;
        std::array<bool, 3> rotationAxesMixed{};
        TransformRow world;
        std::vector<ComponentChoice> addable;
        std::vector<AssetRecord> assets;
        std::uint64_t editRevision = 0;
        std::uint64_t savedRevision = 0;

        bool hasUnsavedEdits() const;

        bool hasMixedNames() const;

        bool isInSubtree(Entity entity, Entity root) const;

        bool findRow(Entity entity, std::size_t& out) const;

        Entity getParentOfRow(std::size_t row) const;

        std::size_t getSiblingIndexOfRow(std::size_t row) const;

        bool resolveDrop(Entity dragged, Entity parent, std::size_t gap, std::size_t& position) const;

        bool resolveDrop(std::span<const Entity> dragged, Entity parent, std::size_t gap, std::size_t& position) const;

        void collectTopmost(std::span<const Entity> selection, std::vector<Entity>& out) const;

        void collectAssetsOfType(StringId type, std::vector<std::size_t>& out) const;
    };
}
