#pragma once

#include <TechEngine/core/scene/ComponentRegistry.hpp>
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
#include <string>
#include <string_view>

namespace TechEngineTests {
    inline TechEngine::ComponentRegistry& registerEditComponents(TechEngine::ComponentRegistry& registry) {
        registry.registerComponent<TechEngine::Hierarchy>(TechEngine::Hierarchy::tag);
        registry.registerComponent<TechEngine::Transform>(TechEngine::Transform::tag);
        registry.registerComponent<TechEngine::Name>(TechEngine::Name::tag);
        return registry;
    }

    class EditTestScene {
    private:
        TechEngine::ComponentRegistry m_registry;

    public:
        TechEngine::Scene scene{registerEditComponents(m_registry)};
        TechEngine::EditSession session;

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
            session.buildSnapshot(scene, out);
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
}
