#pragma once

#include "ClientPanel.hpp"
#include "ServerPanel.hpp"
#include "LoggerPanel.hpp"
#include "ContentBrowserPanel.hpp"
#include "systems/System.hpp"

namespace TechEngine {
    class Server;
    class Client;
    class Window;
    class ContentBrowserPanel;

    class PanelsManager : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        Client& m_client;
        Server& m_server;

        ImGuiID m_dockSpaceID;
        ClientPanel m_clientPanel;
        ServerPanel m_serverPanel;
        LoggerPanel m_LoggerPanel;
        ContentBrowserPanel m_ContentBrowserPanel;
        ProfilerPanel m_ProfilerPanel;
        inline static ImGuiWindowClass m_editorWindowClass;

    public:
        PanelsManager(SystemsRegistry& systemsRegistry, Client& client, Server& server);

        void init() override;

        void beginFrame();

        void onUpdate() override;

        void endFrame();

        void shutdown() override;

    private:
        void initImGui();

        void setColorTheme();

        void setupInitialDockingLayout();

        void createDockSpace();

        void drawMenuBar();
    };
}
