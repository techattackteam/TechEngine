#pragma once

#include <ui/panels/GameViewPanel.hpp>
#include <ui/panels/HierarchyPanel.hpp>
#include <ui/panels/InspectorPanel.hpp>
#include <ui/panels/SceneViewPanel.hpp>

#include <imgui.h>

#include <functional>

namespace TechEngine {
    class EditorCommands;
    struct EditorSnapshot;

    class PanelsManager {
    private:
        EditorCommands* m_commands = nullptr;
        std::function<void()> m_exportProject;
        std::function<void()> m_runProject;
        HierarchyPanel m_hierarchy;
        InspectorPanel m_inspector;
        SceneViewPanel m_sceneView;
        GameViewPanel m_gameView;

    public:
        PanelsManager(EditorCommands& commands, std::function<void()> exportProject, std::function<void()> runProject);

        void draw(const EditorSnapshot& snapshot, bool canRun);

    private:
        void drawMainMenuBar(bool canRun);

        void handleShortcuts();

        void drawDockSpace();

        void buildDefaultLayout(ImGuiID dockSpaceId);
    };
}
