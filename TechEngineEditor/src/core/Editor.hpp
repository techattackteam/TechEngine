#pragma once

#include "panels/PanelsManager.hpp"
#include "core/Client.hpp"
#include "core/Server.hpp"


using std::filesystem::path;

namespace TechEngine {
    class Editor : public AppCore {
    private:
        Window window;
        Client client;
        Server server;
        ProjectManager projectManager;
        PanelsManager panelsManager;
        std::string editorSettings;

    public:
        Editor();

        void onUpdate() override;

        void onFixedUpdate() override;

        void loadEditorSettings();

        void createNewProject();
    };
}
