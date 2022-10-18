#pragma once

#include "network/SNetworkHandler.hpp"

namespace TechEngineServer {
    class Engine_API App : public TechEngineCore::App {
    private:
        bool running = true;

    protected:
        SNetworkHandler networkHandler;

    public:

        GameApp();

        ~GameApp() override;

        void run() override;

        virtual void onUpdate() = 0;

        virtual void onFixedUpdate() = 0;

    private:

    };

    App *createApp();
}
