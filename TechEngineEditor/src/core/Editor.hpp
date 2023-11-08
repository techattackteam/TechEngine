#pragma once

#include "panels/PanelsManager.hpp"
#include "core/Window.hpp"
#include "core/App.hpp"
#include "external/EntryPoint.hpp"

using std::filesystem::path;
namespace TechEngine {
    class Editor : public App {
    private:
        Window window{"TechEngine", RendererSettings::width, RendererSettings::height};
        PanelsManager panelsManager;

        path rootPath;
        path editorSettings;
    public:
        Editor();

        void onUpdate() override;

        void onFixedUpdate() override;

        void loadEditorSettings();
    };
}


