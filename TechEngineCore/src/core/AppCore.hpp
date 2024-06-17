#pragma once

#include "Timer.hpp"
#include "CoreExportDll.hpp"
#include "script/ScriptEngine.hpp"
#include "FilePaths.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "material/MaterialManager.hpp"
#include "scene/SceneManager.hpp"
#include "texture/TextureManager.hpp"

namespace TechEngine {
    class CORE_DLL AppCore {
    protected:
        bool running = true;

    public:
        Timer timer;
        EventDispatcher eventDispatcher;
        FilePaths filePaths;
        TextureManager textureManager;
        MaterialManager materialManager;
        SceneManager sceneManager;
        PhysicsEngine physicsEngine;

        ScriptEngine scriptEngine;

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
