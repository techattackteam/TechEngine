#pragma once

#include "core/AppCore.hpp"
#include "Window.hpp"
#include "events/window/WindowCloseEvent.hpp"
#include "material/MaterialManager.hpp"
#include "texture/TextureManager.hpp"
#include "project/ProjectManager.hpp"
#include "scene/SceneManager.hpp"
#include "physics/PhysicsEngine.hpp"
#include "scriptingAPI/TechEngineAPI.hpp"

namespace TechEngine {
    class TECHENGINE_API App : public AppCore {
    public:
        Window window;
        Renderer renderer;
        TextureManager textureManager;
        MaterialManager materialManager;
        SceneManager sceneManager;
        ProjectManager projectManager;
        PhysicsEngine physicsEngine;
        TechEngineAPI api;

        App(std::string name, int width, int height);

        ~App() override;

        void run() override;

    private:
        void onWindowCloseEvent(WindowCloseEvent* event);
    };
}
