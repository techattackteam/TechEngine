#include "EventDispatcherAPI.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "script/ScriptCrashHandler.hpp"

namespace TechEngine {
    EventDispatcherAPI::EventDispatcherAPI(EventDispatcher* eventDispatcher) : eventDispatcher(eventDispatcher) {
        TE_LOGGER_INFO("EventDispatcherAPI created at {0}", (void*)instance);
        instance = this;
        TE_LOGGER_INFO("EventDispatcherAPI instance set to {0} with eventDipatcher {1}", (void*)instance, (void*)instance->eventDispatcher);
    }

    void EventDispatcherAPI::dispatch(Event* event) {
        instance->eventDispatcher->dispatch(event);
    }

    void EventDispatcherAPI::subscribe(const EventType& eventType, const std::function<void(Event*)>& callback) {
        instance->eventDispatcher->subscribe(eventType, callback);
    }
}
