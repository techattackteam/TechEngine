#pragma once

#include <ui/panels/AssetBrowserPanel.hpp>
#include <ui/panels/ConsolePanel.hpp>
#include <ui/panels/GameViewPanel.hpp>
#include <ui/panels/HierarchyPanel.hpp>
#include <ui/panels/InspectorPanel.hpp>
#include <ui/panels/SceneViewPanel.hpp>

#include <imgui.h>

#include <functional>
#include <vector>

namespace TechEngine {
    class ConsoleHistory;
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
        ConsolePanel m_console;
        AssetBrowserPanel m_assetBrowser;
        bool m_sceneViewDrawn = false;

    public:
        PanelsManager(EditorCommands& commands, ConsoleHistory& consoleHistory, std::function<void()> exportProject, std::function<void()> runProject);

        // Call right after the ImGui context is created, before the layout loads.
        void registerSettings();

        // Fills `views` with one request per view panel, hidden or not.
        void draw(const EditorSnapshot& snapshot, bool canRun, std::vector<ViewRequest>& views);

        // Both are false while the scene view is hidden, so a hidden panel never keeps the cursor locked.
        bool wantsCursorLocked() const;

        bool isSceneCameraMoving() const;

    private:
        void drawMainMenuBar(bool canRun);

        void handleShortcuts();

        void drawDockSpace();

        void buildDefaultLayout(ImGuiID dockSpaceId);
    };
}
