#include "scriptingAPI/network/NetworkVariable.hpp"
#include "core/Logger.hpp"
#include "events/network/NetworkIntValueChanged.hpp"
#include "events/network/SyncNetworkInt.hpp"

namespace TechEngine {
    NetworkVariable::NetworkVariable(NetworkObject* networkObject, const std::string& name, int value) : networkObject(networkObject), name(name), value(value) {
        networkObject->registerVariable(name, this);
    }

    NetworkVariable::~NetworkVariable() {
        networkObject->unregisterVariable(name);
    }

    int NetworkVariable::getValue() const {
        return value;
    }

    void NetworkVariable::setValue(int value) {
        //this->value = value;
        networkObject->requestChangeValue(name, value);
    }

    void NetworkVariable::sync(SyncNetworkInt* event) {
        int oldValue = value;
        value = event->getValue();
        TE_LOGGER_INFO("Synced NetworkVariable {0} value: {1}", name, value);
        if (valueChangedCallback) {
            valueChangedCallback(oldValue, value);
        }
    }

    void NetworkVariable::onValueChanged(std::function<void(int, int)> callback) {
        valueChangedCallback = callback;
    }
}
