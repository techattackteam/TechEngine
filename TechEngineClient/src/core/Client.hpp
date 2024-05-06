#pragma once

#include <scriptingAPI/TechEngineAPIClient.hpp>

#include "core/Core.hpp"
#include "core/AppCore.hpp"
#include "Window.hpp"
#include "core/FilePaths.hpp"
#include "events/window/WindowCloseEvent.hpp"
#include "network/NetworkEngine.hpp"


namespace TechEngine {
    class TECHENGINE_API Client : public AppCore {
    public:
        Window& window;
        Renderer renderer;
        NetworkEngine networkEngine;
        TechEngineAPIClient api;
        FilePaths filePaths;

        Client(Window& window);

        ~Client() override;

        void onFixedUpdate() override;

        void onUpdate() override;

    private:
        void onWindowCloseEvent(WindowCloseEvent* event);
    };
}
