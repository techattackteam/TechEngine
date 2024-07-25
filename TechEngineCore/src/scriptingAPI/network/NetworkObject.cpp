#include "NetworkObject.hpp"

#include <utility>

#include "NetworkData.hpp"
#include "events/network/NetworkIntValueChanged.hpp"
#include "events/network/RequestNetworkVariableCreation.hpp"
#include "events/network/SyncNetworkInt.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "scriptingAPI/event/EventDispatcherAPI.hpp"


namespace TechEngine {
    NetworkObject::NetworkObject() {
        EventDispatcherAPI::subscribe(SyncNetworkInt::eventType, [this](Event* event) {
            onSyncNetworkVariable((SyncNetworkInt*)event);
        });
    }

    NetworkObject::~NetworkObject() = default;

    void NetworkObject::registerVariable(const std::string& name, NetworkVariable* variable) {
        if (variables.find(name) == variables.end()) {
            variables[name] = variable;
        }
    }

    void NetworkObject::unregisterVariable(const std::string& name) {
        if (variables.find(name) != variables.end()) {
            variables.erase(name);
        }
    }

    void NetworkObject::requestVariablesCreation() {
        for (auto& variable: variables) {
            EventDispatcherAPI::dispatch(new RequestNetworkVariableCreation(owner, networkUUID, variable.second->name, variable.second->getValue()));
        }
    }

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

    void NetworkObject::requestChangeValue(const std::string& name, int value) {
        EventDispatcherAPI::dispatch(new NetworkIntValueChanged(owner, networkUUID, name, value));
    }

    bool NetworkObject::isOwner() {
        return NetworkData::getNetworkID() == owner;
    }

    void NetworkObject::updateNetworkVariables() {
    }

    void NetworkObject::setOwner(unsigned int owner) {
        this->owner = owner;
    }

    void NetworkObject::setNetworkUUID(std::string networkUUID) {
        this->networkUUID = std::move(networkUUID);
    }

    void NetworkObject::onSyncNetworkVariable(SyncNetworkInt* event) {
        if (event->getNetworkUUID() == networkUUID) {
            if (isVariableRegistered(event->getName())) {
                variables[event->getName()]->sync(event);
            }
        }
    }

    bool NetworkObject::isVariableRegistered(const std::string& name) {
        return variables.find(name) != variables.end();
    }
}
