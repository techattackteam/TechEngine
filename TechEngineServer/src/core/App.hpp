#pragma once

#include "network/SNetworkHandler.hpp"
#include "../../TechEngineCore/src/core/App.hpp"

namespace TechEngineServer {
    class App : public TechEngineCore::App {
    private:
        bool running = true;

    protected:
        SNetworkHandler networkHandler;

    public:

        App();

        ~App() override;

        void run() override;

        virtual void onUpdate() = 0;

        virtual void onFixedUpdate() = 0;

    private:

    };

    App *createApp();
}
