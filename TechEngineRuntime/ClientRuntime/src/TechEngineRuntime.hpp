#pragma once

#include "core/Client.hpp"
#include "core/Window.hpp"
#include "events/window/WindowResizeEvent.hpp"

namespace TechEngine {
    class TechEngineRuntime : public Client {
    private:
        Window window;

    public:
        TechEngineRuntime();

        void init();

    private:
        void onUpdate() override;

        void onFixedUpdate() override;

        bool loadRendererSettings();

        void onWindowResizeEvent(WindowResizeEvent* event);
    };
}
