#pragma once


#include "Window.hpp"
#include "core/AppCore.hpp"
#include "events/window/WindowCloseEvent.hpp"
#include "network/NetworkEngine.hpp"
#include "scriptingAPI/TechEngineAPIClient.hpp"
#include "core/ClientExportDll.hpp"

namespace TechEngine {
    class CLIENT_DLL Client : public AppCore {
    public:
        Window& window;
        FilePaths filePaths;
        //TechEngineAPIClient api;

        Client(Window& window);

        ~Client() override;

        void init();

        void onFixedUpdate() override;

        void onUpdate() override;

    private:
        void onWindowCloseEvent(WindowCloseEvent* event);
    };
}
