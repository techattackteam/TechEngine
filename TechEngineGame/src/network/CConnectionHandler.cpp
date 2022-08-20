#include "CConnectionHandler.hpp"

namespace TechEngine {
    CConnectionHandler::CConnectionHandler(CNetworkHandler *networkHandler) : networkHandler(networkHandler), ConnectionHandler(networkHandler) {

    }

    CConnectionHandler::~CConnectionHandler() {
    }

    void CConnectionHandler::connectToServer() {
        requestConnection();
    }

    void CConnectionHandler::requestConnection() {
        networkHandler->getPacketHandler().sendPacket(new ConnectionRequestPacket(), networkHandler->getServerEndpoint());
    }
}
