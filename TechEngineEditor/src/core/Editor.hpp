#pragma once

#include "panels/PanelsManager.hpp"
#include "SceneCamera.hpp"
#include "core/Window.hpp"

#include "testGameObject/QuadMeshTest.hpp"
#include "core/App.hpp"
#include "external/EntryPoint.hpp"

namespace TechEngine {
    class Editor : public TechEngine::App {
    private:
        Window window{"TechEngine", RendererSettings::width, RendererSettings::height};
        PanelsManager panelsManager{window};
        SceneCamera sceneCamera{};
    public:
        Editor();

        void onUpdate() override;

        void onFixedUpdate() override;
    };
}


