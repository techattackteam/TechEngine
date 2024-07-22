#include "NetworkObject.hpp"

#include <utility>


namespace TechEngine {
    NetworkObject::~NetworkObject() = default;

    void NetworkObject::onUpdate() {
    }

    void NetworkObject::onFixedUpdate() {
    }

    unsigned int NetworkObject::getOwner() const {
        return owner;
    }

    const std::string& NetworkObject::getNetworkUUID() const {
        return networkUUID;
    }

    void NetworkObject::setOwner(unsigned int owner) {
        this->owner = owner;
    }

    void NetworkObject::setNetworkUUID(std::string networkUUID) {
        this->networkUUID = std::move(networkUUID);
    }
}
