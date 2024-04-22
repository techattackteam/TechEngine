#include "EventDispatcherAPI.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "script/ScriptCrashHandler.hpp"

namespace TechEngine {
    EventDispatcherAPI::EventDispatcherAPI(EventDispatcher* eventDispatcher) {
        EventDispatcherAPI::eventDispatcher = eventDispatcher;
    }

    EventDispatcherAPI::~EventDispatcherAPI() {
    }

    void EventDispatcherAPI::dispatch(Event* event) {
        eventDispatcher->dispatch(event);
    }

    void EventDispatcherAPI::subscribe(EventType eventType, const std::function<void(Event*)>& callback) {
        eventDispatcher->subscribe(eventType, CATCH_EXCEPTION_IN_FUNCTION(callback));
    }
}
