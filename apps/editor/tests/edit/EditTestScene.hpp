#pragma once

#include <TechEngine/base/stringid/StringId.hpp>
#include <TechEngine/core/resources/AssetRef.hpp>
#include <TechEngine/core/scene/ComponentRegistry.hpp>
#include <TechEngine/core/scene/ComponentTypeId.hpp>
#include <TechEngine/core/scene/Entity.hpp>
#include <TechEngine/core/scene/Scene.hpp>
#include <TechEngine/core/scene/components/Hierarchy.hpp>
#include <TechEngine/core/scene/components/Name.hpp>
#include <TechEngine/core/scene/components/Transform.hpp>

#include <edit/EditSession.hpp>
#include <edit/EditorSnapshot.hpp>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace TechEngineTests {
    struct InspectedAsset {};

    struct InspectedSample {
        static constexpr std::string_view tag = "TechEngineTests::InspectedSample";

        bool enabled = true;
        std::int32_t count = 3;
        std::uint8_t layer = 1;
        double weight = 0.5;
        std::string label = "Sample";
        TechEngine::StringId group = TechEngine::StringId("Group");
        TechEngine::Entity target;
        TechEngine::AssetRef<InspectedAsset> asset;
        std::vector<std::uint32_t> indices = {1, 2, 3};

        template<typename Archive>
        friend void visit(Archive& archive, InspectedSample& value) {
            archive.field("enabled", value.enabled);
            archive.field("count", value.count);
            archive.field("layer", value.layer);
            archive.field("weight", value.weight);
            archive.field("label", value.label);
            archive.field("group", value.group);
            archive.field("target", value.target);
            archive.field("asset", value.asset);
            archive.field("indices", value.indices);
        }
    };

    inline constexpr std::uint32_t SAMPLE_ENABLED_FIELD = 0;
    inline constexpr std::uint32_t SAMPLE_COUNT_FIELD = 1;
    inline constexpr std::uint32_t SAMPLE_LAYER_FIELD = 2;
    inline constexpr std::uint32_t SAMPLE_WEIGHT_FIELD = 3;
    inline constexpr std::uint32_t SAMPLE_LABEL_FIELD = 4;
    inline constexpr std::uint32_t SAMPLE_GROUP_FIELD = 5;
    inline constexpr std::uint32_t SAMPLE_TARGET_FIELD = 6;
    inline constexpr std::uint32_t SAMPLE_ASSET_FIELD = 7;
    inline constexpr std::uint32_t SAMPLE_INDICES_FIELD = 8;
    inline constexpr std::uint32_t SAMPLE_FIELD_COUNT = 9;

    inline constexpr std::uint32_t TRANSFORM_POSITION_X_FIELD = 0;
    inline constexpr std::uint32_t TRANSFORM_POSITION_Y_FIELD = 1;
    inline constexpr std::uint32_t TRANSFORM_SCALE_X_FIELD = 7;
    inline constexpr std::uint32_t TRANSFORM_FIELD_COUNT = 10;

    inline TechEngine::ComponentRegistry& registerEditComponents(TechEngine::ComponentRegistry& registry) {
        registry.registerComponent<TechEngine::Hierarchy>(TechEngine::Hierarchy::tag);
        registry.registerComponent<TechEngine::Transform>(TechEngine::Transform::tag);
        registry.registerComponent<TechEngine::Name>(TechEngine::Name::tag);
        registry.registerComponent<InspectedSample>(InspectedSample::tag);
        return registry;
    }

    class EditTestScene {
    private:
        TechEngine::ComponentRegistry m_registry;

    public:
        TechEngine::Scene scene{registerEditComponents(m_registry)};
        TechEngine::EditSession session;

        const TechEngine::ComponentRegistry& getRegistry() const {
            return m_registry;
        }

        TechEngine::Entity createNamed(const std::string_view name, const TechEngine::Entity parent = {}) {
            const TechEngine::Entity entity = scene.createEntity();
            REQUIRE(entity.valid());
            scene.getComponent<TechEngine::Name>(entity).value = std::string(name);
            if (parent.valid()) {
                const bool parented = scene.setParent(entity, parent, scene.getChildren(parent).size());
                REQUIRE(parented);
            }
            return entity;
        }

        TechEngine::EditorSnapshot snapshot() const {
            TechEngine::EditorSnapshot out;
            session.buildSnapshot(scene, m_registry, out);
            return out;
        }
    };

    inline std::size_t rowIndex(const TechEngine::EditorSnapshot& snapshot, const TechEngine::Entity entity) {
        const auto row = std::ranges::find(snapshot.hierarchy, entity, &TechEngine::HierarchyRow::entity);
        REQUIRE(row != snapshot.hierarchy.end());
        return static_cast<std::size_t>(row - snapshot.hierarchy.begin());
    }

    inline const TechEngine::HierarchyRow& rowFor(const TechEngine::EditorSnapshot& snapshot, const TechEngine::Entity entity) {
        return snapshot.hierarchy[rowIndex(snapshot, entity)];
    }

    inline const TechEngine::ComponentRow* findComponentRow(const TechEngine::EditorSnapshot& snapshot, const TechEngine::ComponentTypeId type) {
        const auto row = std::ranges::find(snapshot.components, type, &TechEngine::ComponentRow::type);
        if (row == snapshot.components.end()) {
            return nullptr;
        }
        return &*row;
    }

    inline const TechEngine::ComponentRow& componentRowFor(const TechEngine::EditorSnapshot& snapshot, const TechEngine::ComponentTypeId type) {
        const TechEngine::ComponentRow* row = findComponentRow(snapshot, type);
        REQUIRE(row != nullptr);
        return *row;
    }
}
