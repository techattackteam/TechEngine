#pragma once

#include <TechEngine/base/math/Math.hpp>
#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/ResourceId.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>

#include <assets/index/AssetIndex.hpp>
#include <edit/fields/FieldValue.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
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
        // Copies of the asset index and its listing, which main must not read while the simulation thread may rebuild them.
        std::vector<AssetRecord> assets;
        std::vector<ListedEntry> listing;
        std::vector<SideFolder> sideFolders;
        std::uint64_t listingRevision = 0;
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

        // The asset an inspector field of type takes from a drop of paths. False unless paths holds one path, and the
        // record at that path is one the picker for type would offer.
        bool findDroppedAsset(std::span<const std::string> paths, StringId type, ResourceId& out) const;

        // The paths a drop places, in the order given: those listed as a model or a mesh.
        void collectPlaceable(std::span<const std::string> paths, std::vector<std::string>& out) const;

        static bool isPlaceable(const ListedEntry& entry);

        // Indices into listing of folder's own children: folders first, then files, each by name without case.
        void collectFolderContents(std::string_view folder, std::vector<std::size_t>& out) const;

        // out is folder when it is a side folder or a listed folder, else its closest ancestor that is. False when
        // folder is under no side folder.
        bool findNearestFolder(std::string_view folder, std::string& out) const;

        // Each folder path from folder's side folder down to folder. Empty when folder is under no side folder.
        void collectBreadcrumbs(std::string_view folder, std::vector<std::string>& out) const;

        // The side folder that is path or holds it, the longest one when side folders nest. False when path is under no
        // side folder.
        bool findSideFolder(std::string_view path, SideFolder& out) const;

        // The first of candidate(0), candidate(1), ... that no listed entry holds, file or folder. A create in flight
        // that this listing does not show yet may take it first.
        std::string firstFreePath(const std::function<std::string(std::uint32_t suffix)>& candidate) const;

    private:
        static bool isOfferedFor(const AssetRecord& record, StringId type);
    };
}
