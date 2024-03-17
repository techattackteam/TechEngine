#include "EventDispatcherAPI.hpp"
#include "event/EventDispatcher.hpp"

namespace TechEngine {
    EventDispatcherAPI::EventDispatcherAPI(EventDispatcher* eventDispatcher) {
        EventDispatcherAPI::eventDispatcher = eventDispatcher;
    }

    void EventDispatcherAPI::dispatch(Event* event) {
        eventDispatcher->dispatch(event);
    }

    void EventDispatcherAPI::subscribe(EventType eventType, const std::function<void(Event*)>& callback) {
        eventDispatcher->subscribe(eventType, callback);
    }
}
