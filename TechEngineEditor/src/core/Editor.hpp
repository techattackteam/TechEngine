#pragma once

#include <TechEngine.hpp>
#include <EntryPoint.hpp>
#include "panels/PanelsManager.hpp"
#include "SceneCamera.hpp"
#include "core/Window.hpp"

#include "testGameObject/QuadMeshTest.hpp"

namespace TechEngine {
    class Editor : public TechEngine::App {
    private:
        Window window{"Main", 1200, 600};
        PanelsManager panelsManager{window};
        SceneCamera sceneCamera{};
    public:
        Editor();

        void onUpdate() override;

        void onFixedUpdate() override;
    };
}


