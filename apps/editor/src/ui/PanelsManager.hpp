#pragma once

#include <ui/panels/ConsolePanel.hpp>
#include <ui/panels/GameViewPanel.hpp>
#include <ui/panels/HierarchyPanel.hpp>
#include <ui/panels/InspectorPanel.hpp>
#include <ui/panels/SceneViewPanel.hpp>

#include <imgui.h>

#include <array>
#include <cstddef>
#include <functional>
#include <string>

namespace TechEngine {
    class ConsoleHistory;
    class EditorCommands;
    struct EditorSnapshot;

    class PanelsManager {
    private:
        static constexpr std::size_t PLACE_MODEL_PATH_CAPACITY = 512;
        static constexpr std::size_t IMPORT_IMAGE_PATH_CAPACITY = 512;

        EditorCommands* m_commands = nullptr;
        std::function<void()> m_exportProject;
        std::function<void()> m_runProject;
        std::function<void(std::string)> m_importImage;
        HierarchyPanel m_hierarchy;
        InspectorPanel m_inspector;
        SceneViewPanel m_sceneView;
        GameViewPanel m_gameView;
        ConsolePanel m_console;
        bool m_placeModelRequested = false;
        std::array<char, PLACE_MODEL_PATH_CAPACITY> m_placeModelPath{};
        bool m_importImageRequested = false;
        std::array<char, IMPORT_IMAGE_PATH_CAPACITY> m_importImagePath{};

    public:
        PanelsManager(EditorCommands& commands, ConsoleHistory& consoleHistory, std::function<void()> exportProject, std::function<void()> runProject, std::function<void(std::string)> importImage);

        void draw(const EditorSnapshot& snapshot, bool canRun);

    private:
        void drawMainMenuBar(bool canRun);

        void drawPlaceModelPopup();

        void drawImportImagePopup();

        void handleShortcuts();

        void drawDockSpace();

        void buildDefaultLayout(ImGuiID dockSpaceId);
    };
}
