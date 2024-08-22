#pragma once

#include "ClientPanel.hpp"
#include "ServerPanel.hpp"
#include "systems/System.hpp"

namespace TechEngine {
    class Window;

    class PanelsManager : public System {
    private:
        SystemsRegistry& m_systemsRegistry;
        ImGuiID m_dockSpaceID;
        ClientPanel m_ClientPanel;
        ServerPanel m_ServerPanel;
    public:
        inline static ImGuiWindowClass m_editorWindowClass;

        PanelsManager(SystemsRegistry& systemsRegistry);

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
