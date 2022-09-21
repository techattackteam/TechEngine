#include "SSocketHandler.hpp"

namespace TechEngineServer {
    SSocketHandler::SSocketHandler(SNetworkHandler *networkHandler) : networkHandler(networkHandler), SocketHandler(networkHandler) {

    }

    void SSocketHandler::init() {
        socket = new udp::socket(context, udp::endpoint(udp::v4(), networkHandler->getPort()));
        SocketHandler::init();
    }
}