#pragma once

#include "Panel.hpp"
#include "components/Archetype.hpp"

namespace TechEngine {
    class Tag;

    class SceneHierarchyPanel : public Panel {
        SystemsRegistry& m_appSystemRegistry;
        std::vector<Entity>& m_selectedEntities;
        std::vector<Tag> entitiesOrder;
        bool isItemHovered = false;

    public:
        SceneHierarchyPanel(SystemsRegistry& editorSystemRegistry, SystemsRegistry& appSystemRegistry, std::vector<Entity>& selectedEntities);

        void onInit() override;

        void onUpdate() override;


        void drawEntityNode(Tag& tag);

        void handleReorder(Entity draggedEntity, size_t newPosition);

        void drawDropZone(size_t position);

        void reorderEntity(Tag& tag, size_t newPosition);
    };
}
