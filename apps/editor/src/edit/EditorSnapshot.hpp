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
        // Depth-first: a row's subtree is the run of rows after it with a greater depth. Roots come
        // in storage order, which changes when an entity is destroyed or moves to another archetype.
        std::vector<HierarchyRow> hierarchy;
        std::vector<Entity> selection;
        Entity active;
        // Main draws its own set, not this one, until this reaches the sequence main last sent.
        std::uint64_t selectionSequence = 0;
        // Name is left out: the inspector renames through RenameEntityCommand, never a set-field command.
        std::vector<ComponentRow> components;
        // Euler X, Y and Z of Transform's rotation. The quaternion fields' own mixed flags say nothing about one axis.
        std::array<bool, 3> rotationAxesMixed{};
        // The active entity's world values, filled only while components lists Transform.
        TransformRow world;
        std::vector<ComponentChoice> addable;
        // A copy of the asset index, which main must not read while the simulation thread may add to it.
        std::vector<AssetRecord> assets;
        std::uint64_t editRevision = 0;
        std::uint64_t savedRevision = 0;

        bool hasUnsavedEdits() const;

        bool hasMixedNames() const;

        bool isInSubtree(Entity entity, Entity root) const;

        bool findRow(Entity entity, std::size_t& out) const;

        Entity getParentOfRow(std::size_t row) const;

        std::size_t getSiblingIndexOfRow(std::size_t row) const;

        // gap counts parent's children as drawn, dragged included; Scene counts a position among
        // the other children, so a move down inside the same parent lands one gap earlier.
        bool resolveDrop(Entity dragged, Entity parent, std::size_t gap, std::size_t& position) const;

        // dragged is in hierarchy order. position counts among parent's children that are not dragged. A drop at the
        // root is allowed while any dragged entity is a child, and its position is 0.
        bool resolveDrop(std::span<const Entity> dragged, Entity parent, std::size_t gap, std::size_t& position) const;

        // Hierarchy order; an entity with a selected ancestor, or one this snapshot does not list, is left out.
        void collectTopmost(std::span<const Entity> selection, std::vector<Entity>& out) const;

        // Indices into assets, in index order. An empty type matches nothing, and a server asset is left out, because
        // the set command refuses it.
        void collectAssetsOfType(StringId type, std::vector<std::size_t>& out) const;
    };
}
