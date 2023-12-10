#pragma once

#include "core/Window.hpp"
#include "core/App.hpp"
#include "external/EntryPoint.hpp"

namespace TechEngine {
    class TechEngineRuntime : public App {
    private:
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

        void onWindowResizeEvent(WindowResizeEvent *event);
    };
}
