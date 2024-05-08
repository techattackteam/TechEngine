#pragma once

#include "Timer.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "physics/PhysicsEngine.hpp"
#include "scene/SceneManager.hpp"
#include "script/ScriptEngine.hpp"
#include "scriptingAPI/TechEngineAPI.hpp"

namespace TechEngine {
    class AppCore {
    protected:
        bool running = true;

    public:
        EventDispatcher eventDispatcher;
        Timer timer;
        FilePaths filePaths;
        TextureManager textureManager;
        MaterialManager materialManager;
        SceneManager sceneManager;
        PhysicsEngine physicsEngine;
        ScriptEngine scriptEngine;
        TechEngineAPI api;

        AppCore();

        virtual ~AppCore();

        void run();

        virtual void onUpdate() = 0;

        virtual void onFixedUpdate() = 0;

    private:
        void onAppCloseRequestEvent();
    };

    AppCore* createApp();
};
