#include "SPacketHandler.hpp"

namespace TechEngineServer {
    SPacketHandler::SPacketHandler(SNetworkHandler *networkHandler) : PacketHandler(networkHandler) {
    }

    void SPacketHandler::init() {
        PacketHandler::init();
    }

/*
    void SPacketHandler::onPlayerJoinEvent(PlayerJoinEvent *event) {
        for (auto &element : ((SNetworkHandler *) networkHandler)->getClients()) {
            if (event->getUUID() != element.first) {
                networkHandler->getPacketHandler()->sendPacket(new PlayerJoinPacket(element.first, event->getUUID(), event->getPlayer()->getPosition()), element.second->endpoint);
            }
        }
    }
*/
}