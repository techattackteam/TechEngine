#pragma once

#include "core/AppCore.hpp"
#include "Window.hpp"
#include "events/window/WindowCloseEvent.hpp"
#include "material/MaterialManager.hpp"
#include "renderer/TextureManager.hpp"
#include "project/ProjectManager.hpp"
#include "scene/SceneManager.hpp"
#include "physics/PhysicsEngine.hpp"

namespace TechEngine {
    class /*Engine_API*/ App : public TechEngine::AppCore {
    public:
        Window window;
        TextureManager textureManager;
        MaterialManager materialManager;
        SceneManager sceneManager;
        ProjectManager projectManager;
        PhysicsEngine physicsEngine;

        App(std::string name, int width, int height);

        ~App() override;

        void run() override;

    private:
        void onWindowCloseEvent(WindowCloseEvent* event);
    };
}
