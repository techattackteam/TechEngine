#include "EventType.hpp"

#include <utility>

EventType::EventType(std::string name, EventTiming time) {
    this->name = std::move(name);
    this->timing = time;
}

EventName EventType::getName() const {
    return name;
}

EventTiming EventType::getTiming() const {
    return timing;
}
