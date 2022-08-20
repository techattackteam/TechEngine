#include "CSocketHandler.hpp"

namespace TechEngine {
    CSocketHandler::CSocketHandler(TechEngineCore::NetworkHandler *networkHandler) : SocketHandler(networkHandler) {
    }

    CSocketHandler::~CSocketHandler() {
        delete (resolver);
    }

    void CSocketHandler::init() {
        resolver = new udp::resolver(context);
        socket = new udp::socket(context, udp::endpoint(udp::v4(), networkHandler->getPort() + 1));
        SocketHandler::init();
    }

    udp::resolver *CSocketHandler::getResolver() const {
        return resolver;
    }
}
