#pragma once

#include "ClientPanel.hpp"
#include "ServerPanel.hpp"
#include "LoggerPanel.hpp"
#include "ContentBrowserPanel.hpp"
#include "systems/System.hpp"

namespace TechEngine {
    class ContentBrowserPanel;
}

namespace TechEngine {
    class Server;
    class Client;
    class Window;

    class PanelsManager : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        Client& m_client;
        Server& m_server;

        ImGuiID m_dockSpaceID;
        ClientPanel m_ClientPanel;
        ServerPanel m_ServerPanel;
        LoggerPanel m_LoggerPanel;
        ContentBrowserPanel m_ContentBrowserPanel;
        inline static ImGuiWindowClass m_editorWindowClass;

    public:
        PanelsManager(SystemsRegistry& systemsRegistry, Client& client, Server& server);

        void init() override;

        void onUpdate() override;

        void shutdown() override;

    private:
        void initImGui();

        void setColorTheme();

        void setupInitialDockingLayout();

        void createDockSpace();

        void drawMenuBar();
    };
}
