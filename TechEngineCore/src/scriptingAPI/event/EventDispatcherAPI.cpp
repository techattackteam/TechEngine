#include "EventDispatcherAPI.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "script/ScriptCrashHandler.hpp"

namespace TechEngine {
    void EventDispatcherAPI::init(EventDispatcher* clientEventDispatcher, EventDispatcher* serverEventDispatcher) {
        EventDispatcherAPI::clientEventDispatcher = clientEventDispatcher;
        EventDispatcherAPI::serverEventDispatcher = serverEventDispatcher;
    }

    void EventDispatcherAPI::dispatch(Type type, Event* event) {
        if (type == Type::CLIENT) {
            clientEventDispatcher->dispatch(event);
        } else if (type == Type::SERVER) {
            serverEventDispatcher->dispatch(event);
        } else {
            clientEventDispatcher->dispatch(event);
            serverEventDispatcher->dispatch(event);
        }
    }

    void EventDispatcherAPI::subscribe(Type type, const EventType& eventType, const std::function<void(Event*)>& callback) {
        if (type == Type::CLIENT) {
            clientEventDispatcher->subscribe(eventType, CATCH_EXCEPTION_IN_FUNCTION(callback, *clientEventDispatcher));
        } else if (type == Type::SERVER) {
            serverEventDispatcher->subscribe(eventType, CATCH_EXCEPTION_IN_FUNCTION(callback, *serverEventDispatcher));
        } else {
            clientEventDispatcher->subscribe(eventType, CATCH_EXCEPTION_IN_FUNCTION(callback, *clientEventDispatcher));
            serverEventDispatcher->subscribe(eventType, CATCH_EXCEPTION_IN_FUNCTION(callback, *serverEventDispatcher));
        }
    }
}
