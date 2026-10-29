#pragma once

#include <ui/panels/HierarchyPanel.hpp>

#include <imgui.h>

namespace TechEngine {
    class EditorCommands;
    struct EditorSnapshot;

    class PanelsManager {
    private:
        HierarchyPanel m_hierarchy;

    public:
        explicit PanelsManager(EditorCommands& commands);

        void draw(const EditorSnapshot& snapshot);

    private:
        void drawDockSpace();

        void buildDefaultLayout(ImGuiID dockSpaceId);
    };
}
