#pragma once
#include "ExportDLL.hpp"
#include "systems/SystemManager.hpp"

namespace TechEngine {
    class SERVER_DLL Server {
    private:
        SystemManager systemManager; //?

    public:
        void init();

        void onFixedUpdate();

        void onUpdate();

        void destroy();
    };
}
