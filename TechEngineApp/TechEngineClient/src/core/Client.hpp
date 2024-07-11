#pragma once

#include "core/ClientExportDll.hpp"
#include "Window.hpp"
#include "core/AppCore.hpp"
#include "events/window/WindowCloseEvent.hpp"
#include "project/Project.hpp"
#include "scriptingAPI/TechEngineAPIClient.hpp"

namespace TechEngine {
    class CLIENT_DLL Client : public AppCore {
    public:
        Window& window;
        Project project;
        TechEngineAPIClient api;

        Client(Window& window);

        ~Client() override;

        void init();

        void onFixedUpdate() override;

        void onUpdate() override;

    private:
        void onWindowCloseEvent(WindowCloseEvent* event);
    };
}
