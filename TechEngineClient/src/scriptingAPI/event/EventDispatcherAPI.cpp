#include "EventDispatcherAPI.hpp"
#include "event/EventDispatcher.hpp"

namespace TechEngine {
    EventDispatcherAPI::EventDispatcherAPI(TechEngine::EventDispatcher* eventDispatcher) {
        EventDispatcherAPI::eventDispatcher = eventDispatcher;
    }

    void EventDispatcherAPI::dispatch(Event* event) {
        eventDispatcher->dispatch(event);
    }
}
