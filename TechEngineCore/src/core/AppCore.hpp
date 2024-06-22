#pragma once

#include "CoreExportDll.hpp"
#include "FilePaths.hpp"
#include "scene/SceneManager.hpp"

namespace TechEngine {
    class CORE_DLL AppCore {
    public:
        bool running = true;
        SystemsRegistry systemsRegistry;
        FilePaths filePaths;

    public:
        AppCore();

        virtual void init();

        virtual ~AppCore();

        void run();

        virtual void onUpdate() = 0;

        virtual void onFixedUpdate() = 0;

    private:
        void onAppCloseRequestEvent();
    };
};
