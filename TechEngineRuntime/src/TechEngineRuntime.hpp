#pragma once

#include "core/App.hpp"
#include "core/Window.hpp"
#include "external/EntryPoint.hpp"

namespace TechEngine {
    class TechEngineRuntime : public App {
    private:
        Window window;
        std::string sceneToLoadName;
        std::string windowName;
        uint32_t width;
        uint32_t height;
    public:
        TechEngineRuntime();

    private:

        void onUpdate() override;

        void onFixedUpdate() override;

        bool loadRendererSettings();
    };
}
