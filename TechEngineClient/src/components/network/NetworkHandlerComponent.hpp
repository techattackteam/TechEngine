#pragma once

#include "components/Component.hpp"
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

        void sendPacket(TechEngine::Packet *packet);

        void update() override;

        const std::string &getUUID();

        Component* copy(GameObject* gameObjectToAttach, Component* componentToCopy) override;
    };
}
