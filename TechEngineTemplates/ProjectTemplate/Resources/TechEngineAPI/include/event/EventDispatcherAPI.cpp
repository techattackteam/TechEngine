#include "EventDispatcherAPI.hpp"
#include "event/EventDispatcher.hpp"

namespace TechEngineAPI {
    EventDispatcherAPI::EventDispatcherAPI(TechEngine::EventDispatcher* eventDispatcher) {
        EventDispatcherAPI::eventDispatcher = eventDispatcher;
    }

    void EventDispatcherAPI::dispatch(EventAPI* event) {
        eventDispatcher->dispatch(event->getEvent());
    }
}
