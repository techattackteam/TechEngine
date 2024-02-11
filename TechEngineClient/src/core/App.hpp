#pragma once

#include "core/AppCore.hpp"
#include "Window.hpp"
#include "events/window/WindowCloseEvent.hpp"
#include "material/MaterialManager.hpp"
#include "renderer/TextureManager.hpp"
#include "project/ProjectManager.hpp"
#include "scene/SceneManager.hpp"
#include "physics/PhysicsEngine.hpp"
#include "ScriptingAPI/TechEngineAPI.hpp"

namespace TechEngine {
    class TECHENGINE_API App : public AppCore {
    public:
        Window window;
        TextureManager textureManager;
        MaterialManager materialManager;
        SceneManager sceneManager;
        ProjectManager projectManager;
        PhysicsEngine physicsEngine;
        TechEngineAPI::TechEngineAPI api;

        App(std::string name, int width, int height);

        ~App() override;

        void run() override;

    private:
        void onWindowCloseEvent(WindowCloseEvent* event);
    };
}
