#pragma once

#include <filesystem>
#include "Panel.hpp"
#include "project/ProjectManager.hpp"
#include "renderer/ShadersManager.hpp"

namespace TechEngine {
    class PanelsManager;
    enum CompileMode;

    class ExportSettingsPanel : public Panel {
    private:
        int32_t width = 1080;
        int32_t height = 720;

        PanelsManager& panelsManager;
        Client& client;
        Server& server;
        SystemsRegistry& editorRegistry;

    public:
        ExportSettingsPanel(SystemsRegistry& editorRegistry, Client& client, Server& server, PanelsManager& panelsManager);

        ~ExportSettingsPanel() override;

        void init() override;

        void onUpdate() override;

        void exportGameProject(CompileMode compileMode);

        void exportServerProject(CompileMode compileMode);
    };
}
