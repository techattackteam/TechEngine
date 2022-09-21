#pragma once

#include "../Component.hpp"
#include "../../network/CNetworkHandler.hpp"

namespace TechEngine {
    class NetworkHandlerComponent : public Component {
    private:
        CNetworkHandler networkHandler;
        int port = 25565;
    public:
        explicit NetworkHandlerComponent(GameObject *gameObject);

        ~NetworkHandlerComponent();

        void connectionWithServer();

        void update() override;

        void getInfo() override;

        static ComponentName getName() {
            return "NetworkHandlerComponent";
        }
    };
}
