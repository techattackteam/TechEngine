#include "NetworkHandler.hpp"

NetworkHandler::NetworkHandler() : TechEngine::GameObject("NetworkHandler", false) {
    addComponent<TechEngine::NetworkHandlerComponent>(this);
}


