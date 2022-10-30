#include "NetworkHandler.hpp"
#include "components/network/NetworkHandlerComponent.hpp"

NetworkHandler::NetworkHandler() : TechEngine::GameObject("NetworkHandler", false) {
    addComponent<TechEngine::NetworkHandlerComponent>(this);
}


