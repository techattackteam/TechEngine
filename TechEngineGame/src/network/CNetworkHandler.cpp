#include "CNetworkHandler.hpp"

#include <utility>
#include "CSocketHandler.hpp"
#include "CConnectionHandler.hpp"
#include "CPacketHandler.hpp"

namespace TechEngine {
    CNetworkHandler::CNetworkHandler(std::string serverIP, const short &serverPort, const short &port) : serverIP(serverIP), serverPort(serverPort), port(port),
                                                                                                         NetworkHandler(new CSocketHandler(this),
                                                                                                                        new CConnectionHandler(this),
                                                                                                                        new CPacketHandler(this)) {

    }

    void CNetworkHandler::init() {
        std::stringstream ss;
        ss << serverPort;
        serverEndpoint = ((CSocketHandler *) getSocketsHandler())->getResolver()->resolve(udp::v4(), serverIP, ss.str().c_str())->endpoint();
    }


    void CNetworkHandler::connectWithServer() {
        ((CSocketHandler *) socketHandler)->init();
        //connectionHandler->init();
        ((CPacketHandler *) packetHandler)->init();
        init();
        ((CConnectionHandler *) connectionHandler)->connectToServer();
    }

    udp::endpoint CNetworkHandler::getServerEndpoint() const {
        return serverEndpoint;
    }

    void CNetworkHandler::setUUID(std::string uuid) {
        this->uuid = uuid;
    }

    const std::string &CNetworkHandler::getUUID() {
        return uuid;
    }

    const short &CNetworkHandler::getPort() {
        return port;
    }

    void CNetworkHandler::setPort(short port) {
        this->port = port;
    }
}
