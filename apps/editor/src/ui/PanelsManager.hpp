#pragma once

#include <ui/panels/HierarchyPanel.hpp>
#include <ui/panels/InspectorPanel.hpp>

#include <imgui.h>

namespace TechEngine {
    class EditorCommands;
    struct EditorSnapshot;

    class PanelsManager {
    private:
        HierarchyPanel m_hierarchy;
        InspectorPanel m_inspector;

    public:
        explicit PanelsManager(EditorCommands& commands);

        void draw(const EditorSnapshot& snapshot);

    private:
        void drawDockSpace();

        void buildDefaultLayout(ImGuiID dockSpaceId);
    };
}
