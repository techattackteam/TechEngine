#pragma once

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define NOMINMAX

#include "core/Window.hpp"
#include "SceneHierarchyPanel.hpp"
#include "InspectorPanel.hpp"
#include "SceneView.hpp"
#include "ContentBrowserPanel.hpp"
#include "ExportSettingsPanel.hpp"
#include "events/panels/RegisterCustomPanel.hpp"
#include "core/Key.hpp"
#include "GameView.hpp"
#include "MaterialEditor.hpp"
#include "NetworkHelper.hpp"
#include "scene/SceneManager.hpp"
#include "windows.h"

namespace TechEngine {
    enum CompileMode {
        DEBUG,
        RELEASE,
        RELEASEDEBUG
    };

    class PanelsManager {
    private:
        std::vector<CustomPanel*> customPanels;
        ImGuiContext* imguiContext{};
        Window& window;
        ProjectManager& projectManager;
        SceneManager& sceneManager;
        PhysicsEngine& physicsEngine;
        TextureManager& textureManager;
        MaterialManager& materialManager;
        NetworkEngine& networkEngine;


        GameView gameView;
        //SettingsPanel settingsPanel{};
        ContentBrowserPanel contentBrowser;
        ExportSettingsPanel exportSettingsPanel;
        SceneHierarchyPanel sceneHierarchyPanel;
        SceneView sceneView;
        InspectorPanel inspectorPanel;
        MaterialEditor materialEditor;
        NetworkHelper networkHelper;
        bool m_currentPlaying = false;
        bool isCtrlPressed = false;

        std::vector<PROCESS_INFORMATION> clientProcesses;
        std::vector<PROCESS_INFORMATION> serverProcesses;

    public:
        PanelsManager(Window& window, SceneManager& sceneManager,
                      ProjectManager& projectManager,
                      PhysicsEngine& physicsEngine,
                      TextureManager& textureManager,
                      MaterialManager& materialManager,
                      Renderer& renderer,
                      NetworkEngine& networkEngine);

        void init();

        void update();

        void compileClientUserScripts(CompileMode compileMode);

        void compileServerUserScripts(CompileMode compileMode);

        std::vector<GameObject*>& getSelectedGameObjects();

        void openMaterialEditor(const std::string& materialName, const std::string& filepath);

    private:
        void initImGui();

        void registerCustomPanel(RegisterCustomPanel* event);

        void beginImGuiFrame();

        void createDockSpace();

        void createMenuBar();

        void createToolBar();

        void endImGuiFrame();

        std::string openFileWindow(const char* filter);

        std::string saveFile(const char* filter);

        void startRunningScene();

        void stopRunningScene();

        void runServerProcess();

        void runClientProcess();

        void onCloseProcessEvent(DWORD processId);

        void closeAllProcessEvents();

        void OnKeyPressedEvent(Key& key);

        void OnKeyReleasedEvent(Key& key);

        void setColorTheme();
    };
}
