#include <iostream>
#include "CConnectionHandler.hpp"
#include "network/packets/connection/PingPacket.hpp"

namespace TechEngine {
    CConnectionHandler::CConnectionHandler(CNetworkHandler *networkHandler) : networkHandler(networkHandler), ConnectionHandler(networkHandler) {
        EventDispatcher::getInstance().subscribe(PingEvent::eventType, [this](Event *event) {
            onPingEvent((PingEvent *) event);
        });

        EventDispatcher::getInstance().subscribe(ConnectionSuccessfulEvent::eventType, [this](Event *event) {
            onConnectionSuccessEvent((ConnectionSuccessfulEvent *) event);
        });
    }

    CConnectionHandler::~CConnectionHandler() {
    }

    void CConnectionHandler::connectToServer() {
        requestConnection();
    }

    void CConnectionHandler::requestConnection() {
        networkHandler->getPacketHandler().sendPacket(new ConnectionRequestPacket(), networkHandler->getServerEndpoint());
    }

    void CConnectionHandler::onConnectionSuccessEvent(ConnectionSuccessfulEvent *event) {
        networkHandler->setUUID(event->getUUID());
        std::cout << "Connected with server. My UUID: " << event->getUUID() << std::endl;
    }

    void CConnectionHandler::onPingEvent(PingEvent *event) {
        networkHandler->getPacketHandler().sendPacket(new PingPacket(networkHandler->getUUID()), networkHandler->getServerEndpoint());
    }
}
