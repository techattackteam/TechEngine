#include "NetworkVariable.hpp"

#include <utility>

#include "core/Logger.hpp"
#include "events/network/NetworkIntValueChanged.hpp"
#include "events/network/SyncNetworkInt.hpp"
#include "scriptingAPI/event/EventDispatcherAPI.hpp"

namespace TechEngine {
    template<typename T>
    NetworkVariable<T>::NetworkVariable(NetworkObject* networkObject, std::string name, T value) : name(name), value(value) {
        networkObject->registerVariable(name, value);
        EventDispatcherAPI::subscribe(SyncNetworkInt::eventType, [this](Event* event) {
            sync((SyncNetworkInt*)event);
        });
    }

    template<typename T>
    T NetworkVariable<T>::getValue() const {
        return value;
    }

    template<typename T>
    void NetworkVariable<T>::setValue(T value) {
        this->value = value;
        EventDispatcherAPI::dispatch(new NetworkIntValueChanged(value));
    }

    template<typename T>
    void NetworkVariable<T>::sync(SyncNetworkInt* event) {
        value = event->getValue();
        TE_LOGGER_INFO("Synced NetworkVariable {0} value: {1}", name, value);
    }
}
