
#include <iostream>
#include "SConnectionHandler.hpp"
#include "SNetworkHandler.hpp"
#include "network/packets/connection/ConnectionSuccessfulPacket.hpp"

namespace TechEngineServer {
    SConnectionHandler::SConnectionHandler(SNetworkHandler *networkHandler) : ConnectionHandler(networkHandler) {
        this->networkHandler = networkHandler;
        EventDispatcher::getInstance().subscribe(ConnectionRequestEvent::eventType, [this](Event *event) {
            handleConnectionRequest((ConnectionRequestEvent *) event);
        });

        isAlive = new std::thread(&SConnectionHandler::checkAliveClients, this);
    }

    SConnectionHandler::~SConnectionHandler() {
        isAlive->join();
        delete (isAlive);
    }


    void SConnectionHandler::handleConnectionRequest(ConnectionRequestEvent *event) {
        int uuid = networkHandler->getClients().size() + 1;
        std::stringstream ss;
        ss << uuid;
        Client *client = new Client(ss.str(), event->getEndpoint());
        networkHandler->getClients().insert(std::make_pair(ss.str(), client));
        networkHandler->getPacketHandler().sendPacket(new ConnectionSuccessfulPacket(client->UUID), event->getEndpoint());
        std::cout << "Connection received " << std::endl;
        //EventDispatcher::getInstance()->dispatch(new PlayerJoinEvent(client->UUID, player));
    }

    void SConnectionHandler::checkAlive(const std::string &uuid, std::chrono::system_clock::time_point timeStamp) {
        Client *client = ((SNetworkHandler *) networkHandler)->getClient(uuid);
        std::chrono::duration<double> deltaTime = timeStamp - client->getLastPingTime();
        if (deltaTime.count() > TIMEOUT_DELAY) {
            EventDispatcher::getInstance().dispatch(new DisconnectionEvent(uuid, client->endpoint));
        }
    }

    void SConnectionHandler::checkAliveClients() {
        do {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::chrono::system_clock::time_point timeStamp = std::chrono::system_clock::now();
            for (const auto &element: ((SNetworkHandler *) networkHandler)->getClients()) {
                Client *client = element.second;
                checkAlive(element.first, timeStamp);
                networkHandler->getPacketHandler().queueOutboundPacket(new PingPacket((std::string) element.first), client->endpoint);
            }

        } while (networkHandler->isRunning());
    }

}

