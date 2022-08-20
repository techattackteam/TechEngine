#include "CNetworkHandler.hpp"
#include "CSocketHandler.hpp"
#include "CConnectionHandler.hpp"
#include "CPacketHandler.hpp"

namespace TechEngine {
    CNetworkHandler::CNetworkHandler(std::string serverIp, const short &port) : NetworkHandler(serverIp, port,
                                                                                               new CSocketHandler(this),
                                                                                               new CConnectionHandler(this),
                                                                                               new CPacketHandler(this)) {

    }

    void CNetworkHandler::init() {
        std::stringstream ss;
        ss << port;
        serverEndpoint = ((CSocketHandler *) getSocketsHandler())->getResolver()->resolve(udp::v4(), ip, ss.str().c_str())->endpoint();
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

}
