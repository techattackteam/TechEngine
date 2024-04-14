#pragma once

#include "core/Core.hpp"
#include "core/AppCore.hpp"
#include "Window.hpp"
#include "events/window/WindowCloseEvent.hpp"
#include "network/NetworkEngine.hpp"


namespace TechEngine {
    class TECHENGINE_API Client : public AppCore {
    public:
        Window window;
        Renderer renderer;
        NetworkEngine networkEngine;

        Client(std::string name, int width, int height);

        ~Client() override;

        void run() override;

    private:
        void onWindowCloseEvent(WindowCloseEvent* event);
    };
}
