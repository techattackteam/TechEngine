#pragma once

#include "panels/PanelsManager.hpp"
#include "core/Window.hpp"

#include "core/App.hpp"
#include "external/EntryPoint.hpp"

namespace TechEngine {
    class Editor : public App {
    private:
        Window window{"TechEngine", RendererSettings::width, RendererSettings::height};
        PanelsManager panelsManager{window};
    public:
        Editor();

        void onUpdate() override;

        void onFixedUpdate() override;
    };
}


