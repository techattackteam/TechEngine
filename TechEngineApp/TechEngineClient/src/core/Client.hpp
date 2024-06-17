#pragma once


#include "Window.hpp"
#include "core/AppCore.hpp"
#include "events/window/WindowCloseEvent.hpp"
#include "network/NetworkEngine.hpp"
#include "scriptingAPI/TechEngineAPIClient.hpp"

namespace TechEngine {
    class Client : public AppCore {
    public:
        Window& window;
        Renderer renderer;
        NetworkEngine networkEngine;
        FilePaths filePaths;
        TechEngineAPIClient api;

        Client(Window& window);

        ~Client() override;

        void onFixedUpdate() override;

        void onUpdate() override;

    private:
        void onWindowCloseEvent(WindowCloseEvent* event);
    };
}
