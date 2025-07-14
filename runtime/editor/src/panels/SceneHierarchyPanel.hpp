#pragma once

#include "Panel.hpp"
#include "components/Archetype.hpp"
#include "scene/Scene.hpp"

namespace TechEngine {
    class Tag;


    class SceneHierarchyPanel : public Panel {
        enum EntityType {
            Empty,
            Cube,
            Sphere,
            Cylinder,
            Capsule,
            Camera,
            Light
        };

        struct EntityToCreate {
            std::string name;
            EntityType type;
            Entity parent = -1; // Default to no parent
        };

        SystemsRegistry& m_appSystemRegistry;
        std::vector<Entity>& m_selectedEntities;
        std::vector<Entity> m_entitiesToDelete;
        std::vector<EntityToCreate> m_entitiesToCreate;
        std::vector<Tag> m_entitiesOrder;
        bool m_isItemHovered = false;

    public:
        SceneHierarchyPanel(SystemsRegistry& editorSystemRegistry, SystemsRegistry& appSystemRegistry,
                            std::vector<Entity>& selectedEntities);

        void onInit() override;

        void onUpdate() override;

        void drawEntityNode(Tag& tag);

    private:
        void requestEntitiesCreation();

        void openCreateMenu(std::string title, Entity parent);

        void reorderEntity(Tag& tag, size_t newPosition);
    };
}
