#include "NetworkObjectsManager.hpp"
#include "NetworkObjectsRegistry.hpp"
#include "core/Logger.hpp"
#include "scriptingAPI/network/NetworkObject.hpp"

namespace TechEngine {
    NetworkObject* NetworkObjectsManager::createNetworkObject(const std::string& name, int owner, std::string networkUUID) {
        NetworkObjectsRegistry& registry = *NetworkObjectsRegistry::getInstance();
        if (registry.find(name)) {
            auto factory = registry.getFactory(name);
            NetworkObject* networkObject = factory();
            networkObject->setOwner(owner);
            networkObject->setNetworkUUID(std::move(networkUUID));
            networkObject->requestVariablesCreation();
            networkObjects.push_back(networkObject);
            return networkObject;
        } else {
            TE_LOGGER_WARN("Network object with name {0} not found", name);
            return nullptr;
        }
    }

    bool NetworkObjectsManager::deleteNetworkObject(const std::string& networkUUID) {
        if (!networkObjects.empty()) {
            for (auto it = networkObjects.begin(); it != networkObjects.end(); ++it) {
                if ((*it)->getNetworkUUID() == networkUUID) {
                    delete *it;
                    networkObjects.erase(it);
                    return true;
                }
            }
        }
        return false;
    }

    bool NetworkObjectsManager::registerNetworkVariable(int owner, const std::string& uuid, const std::string& variableName, int value) {
        for (NetworkObject* object: networkObjects) {
            if (object->getOwner() == owner && object->getNetworkUUID() == uuid) {
                object->registerVariable(variableName, new NetworkVariable(object, variableName, value));
                return true;
            }
        }
        return false;
    }

    const std::vector<NetworkObject*>& NetworkObjectsManager::getNetworkObjects() const {
        return networkObjects;
    }

    void NetworkObjectsManager::update() {
        for (NetworkObject* networkObject: networkObjects) {
            networkObject->onUpdate();
        }
    }

    void NetworkObjectsManager::fixedUpdate() {
        for (NetworkObject* networkObject: networkObjects) {
            networkObject->onFixedUpdate();
        }
    }

    void NetworkObjectsManager::onDisconnect() {
        for (auto element: networkObjects) {
            deleteNetworkObject(element->getNetworkUUID());
        }
    }
}
