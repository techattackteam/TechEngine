#pragma once
#include "UIRenderInterface.hpp"
#include "core/ExportDLL.hpp"
#include "UISystemInterface.hpp"
#include <memory>

namespace TechEngine {
    class CLIENT_DLL UIRenderer {
    private:
        SystemsRegistry& m_systemsRegistry;
        std::unique_ptr<UIRenderInterface> m_renderInterface;
        std::unique_ptr<UISystemInterface> m_systemInterface;
        Rml::Context* m_context = nullptr;
        bool m_initialized = false;

    public:
        UIRenderer(SystemsRegistry& systemsRegistry);

        ~UIRenderer();

        void init(bool initGUI);

        void onUpdate();

        void shutdown();

        void processMouseButtonDown(int button, int mods);

        void processMouseButtonUp(int button, int mods);

        void processMouseMove(double xpos, double ypos);

        void processMouseScroll(double yoffset);

        void processKeyChar(unsigned int codepoint);

        Rml::Context* getContext();
    };
}
