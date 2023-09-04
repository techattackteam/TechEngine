#include <iostream>
#include "CConnectionHandler.hpp"
#include "network/packets/connection/PingPacket.hpp"
#include "network/packets/connection/ConnectionRequestPacket.hpp"

namespace TechEngine {
    CConnectionHandler::CConnectionHandler(CNetworkHandler *networkHandler) : networkHandler(networkHandler), ConnectionHandler(networkHandler) {
        TechEngine::EventDispatcher::getInstance().subscribe(TechEngine::PingEvent::eventType, [this](TechEngine::Event *event) {
            onPingEvent((TechEngine::PingEvent *) event);
        });

        TechEngine::EventDispatcher::getInstance().subscribe(TechEngine::ConnectionSuccessfulEvent::eventType, [this](TechEngine::Event *event) {
            onConnectionSuccessEvent((TechEngine::ConnectionSuccessfulEvent *) event);
        });
    }

    CConnectionHandler::~CConnectionHandler() {
    }

    void CConnectionHandler::connectToServer() {
        requestConnection();
    }

    void CConnectionHandler::requestConnection() {
        networkHandler->sendPacket(new TechEngine::ConnectionRequestPacket());
    }

    void CConnectionHandler::onConnectionSuccessEvent(TechEngine::ConnectionSuccessfulEvent *event) {
        networkHandler->setUUID(event->getUUID());
        std::cout << "Connected with server. My UUID: " << event->getUUID() << std::endl;
    }

    void CConnectionHandler::onPingEvent(TechEngine::PingEvent *event) {
        networkHandler->sendPacket(new TechEngine::PingPacket());
    }
}
