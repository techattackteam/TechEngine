#pragma once

#include "core/Client.hpp"
#include "core/Window.hpp"
#include "events/window/WindowResizeEvent.hpp"

namespace TechEngine {
    class TechEngineRuntime : public Client {
    private:
        std::string windowName;
        uint32_t width;
        uint32_t height;

    public:
        TechEngineRuntime();

    private:
        void onUpdate() override;

        void onFixedUpdate() override;

        bool loadRendererSettings();

        void onWindowResizeEvent(WindowResizeEvent* event);
    };
}
