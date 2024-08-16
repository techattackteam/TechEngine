#pragma once
#include "ExportDLL.hpp"
#include "systems/SystemManager.hpp"

namespace TechEngine {
    class CLIENT_DLL Client {
    private:
        SystemManager systemManager; //?

    public:
        void init();

        void onFixedUpdate();

        void onUpdate();

        void destroy();
    };
}
