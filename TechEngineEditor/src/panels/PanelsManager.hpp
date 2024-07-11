#pragma once

#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define NOMINMAX

#include "ClientPanel.hpp"
#include "InspectorPanel.hpp"
#include "ContentBrowserPanel.hpp"
#include "ExportSettingsPanel.hpp"
#include "events/panels/RegisterCustomPanel.hpp"
#include "core/Key.hpp"
#include "MaterialEditor.hpp"
#include "ServerPanel.hpp"
#include "scene/SceneManager.hpp"
#include "windows.h"
#include "core/Client.hpp"
#include "core/Server.hpp"

namespace TechEngine {
    enum class CompileMode {
        DEBUG,
        RELEASE,
        RELEASEDEBUG
    };

    class PanelsManager {
    private:
        std::vector<CustomPanel*> customPanels;
        ImGuiContext* imguiContext{};
        Client& client;
        Server& server;
        SystemsRegistry& systemsRegistry;

        //SettingsPanel settingsPanel{};
        ContentBrowserPanel contentBrowser;
        ExportSettingsPanel exportSettingsPanel;
        ClientPanel clientPanel;
        ServerPanel serverPanel;
        MaterialEditor materialEditor;
        bool isCtrlPressed = false;

        std::vector<PROCESS_INFORMATION> clientProcesses;
        std::vector<PROCESS_INFORMATION> serverProcesses;
        friend class ContentBrowserPanel;

    public:
        PanelsManager(Client& client, Server& server, SystemsRegistry& systemsRegistry);

        void init();

        void update();

        void compileUserScripts(CompileMode compileMode, CompileProject compileProject);

        void openMaterialEditor(const std::string& materialName, const std::string& filepath);

    private:
        void initImGui();

        void registerCustomPanel(RegisterCustomPanel* event);

        void beginImGuiFrame();

        void createDockSpace();

        void createMenuBar();

        void endImGuiFrame();

        std::string openFileWindow(const char* filter);

        std::string saveFile(const char* filter);

        void runServerProcess();

        void runClientProcess();

        void onCloseProcessEvent(DWORD processId);

        void closeAllProcessEvents();

        void OnKeyPressedEvent(Key& key);

        void OnKeyReleasedEvent(Key& key);

        void setColorTheme();
    };
}
