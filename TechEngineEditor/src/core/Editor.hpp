#pragma once

#include "panels/PanelsManager.hpp"
#include "core/Window.hpp"
#include "core/App.hpp"
#include "external/EntryPoint.hpp"

using std::filesystem::path;
namespace TechEngine {
    class Editor : public App {
    private:
        PanelsManager panelsManager;

        std::string editorSettings;
    public:
        Editor();

        void onUpdate() override;

        void onFixedUpdate() override;

        void loadEditorSettings();
    };
}


