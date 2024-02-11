#include "EventAPI.hpp"
#include "event/events/Event.hpp"

namespace TechEngineAPI {
    EventAPI::EventAPI(): event(nullptr) {
    }

    TechEngine::Event* EventAPI::getEvent() {
        return event;
    }
}
