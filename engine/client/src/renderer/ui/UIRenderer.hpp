#pragma once
#include "core/ExportDLL.hpp"

namespace TechEngine {
    class SystemsRegistry;

    class CLIENT_DLL UIRenderer {
    private:
        SystemsRegistry& m_systemsRegistry;
        bool m_initialized = false;

    public:
        UIRenderer(SystemsRegistry& systemsRegistry);

        ~UIRenderer();

        void init(bool initGUI);

        void onUpdate();

        void shutdown();
    };
}
