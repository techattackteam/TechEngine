#pragma once

#include "Panel.hpp"
#include "systems/SystemsRegistry.hpp"

namespace TechEngine {
    class Tag;

    class SceneHierarchyPanel : public Panel {
        SystemsRegistry& m_systemRegistry;
        SystemsRegistry& m_appSystemRegistry;
        std::vector<Tag>& m_selectedEntities;
        bool isItemHovered = false;

    public:
        SceneHierarchyPanel(SystemsRegistry& systemRegistry, SystemsRegistry& appSystemRegistry, std::vector<Tag>& selectedEntities);

        void onInit() override;

        void onUpdate() override;

        void drawEntityNode(Tag& tag);
    };
}
