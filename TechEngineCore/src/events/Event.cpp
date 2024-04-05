#include "Event.hpp"

#include <utility>

namespace TechEngine {
    Event::Event(EventType eventType) : eventType(std::move(eventType)) {
    }

    Event::~Event() = default;

    EventType Event::getEventType() {
        return eventType;
    }
}
