#include "EventDispatcherAPI.hpp"

#include "core/Logger.hpp"
#include "eventSystem/EventDispatcher.hpp"

namespace TechEngine {
    EventDispatcherAPI::EventDispatcherAPI(EventDispatcher* eventDispatcher) : eventDispatcher(eventDispatcher) {
        instance = this;
    }

    void EventDispatcherAPI::dispatch(Event* event) {
        getInstance()->eventDispatcher->dispatch(event);
    }

    void EventDispatcherAPI::subscribe(const EventType& eventType, const std::function<void(Event*)>& callback) {
        TE_LOGGER_INFO("Subscribing to event: {0}", (void*)getInstance());
        getInstance()->eventDispatcher->subscribe(eventType, callback);
    }
}
