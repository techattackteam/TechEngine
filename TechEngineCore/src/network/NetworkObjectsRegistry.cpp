#include "NetworkObjectsRegistry.hpp"

#include "core/Logger.hpp"
#include "scriptingAPI/network/NetworkObject.hpp"
#include <utility>

namespace TechEngine {
    NetworkObjectsRegistry::NetworkObjectsRegistry() {
        instance = this;
    }

    NetworkObjectsRegistry* NetworkObjectsRegistry::getInstance() {
        return instance;
    }

    void NetworkObjectsRegistry::registerNetworkObject(const std::string& name, std::function<NetworkObject*()> factory) {
        if (instance->registry.find(name) == instance->registry.end()) {
            instance->registry[name] = std::move(factory);
            TE_LOGGER_INFO("Network object with name {0} registered", name);
        } else {
            TE_LOGGER_WARN("Network object with name {0} already registered", name);
        }
    }

    bool NetworkObjectsRegistry::find(const std::string& name) const {
        return registry.find(name) != registry.end();
    }

    std::function<NetworkObject*()> NetworkObjectsRegistry::getFactory(const std::string& name) const {
        return registry.at(name);
    }
}
