#pragma once

#include "Panel.hpp"
#include "components/ComponentsManager.hpp"
#include "systems/SystemsRegistry.hpp"


namespace TechEngine {
    class SceneHierarchyPanel : public Panel {
        SystemsRegistry& m_systemRegistry;
        SystemsRegistry& m_appSystemRegistry;
        std::vector<Entity>& m_selectedEntities;
        bool isItemHovered = false;
    public:
        SceneHierarchyPanel(SystemsRegistry& systemRegistry, SystemsRegistry& appSystemRegistry, std::vector<Entity>& selectedEntities);

        void onInit() override;

        void onUpdate() override;

        void drawEntityNode(Entity entity);
    };
}
