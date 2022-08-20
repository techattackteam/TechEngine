#pragma once

#include <string>

using EventName = std::string;

enum EventTiming {
    SYNC, ASYNC, LAZY_ASYNC
};

class EventType {
private:
    EventName name;
    EventTiming timing;

public:
    EventType(std::string name, EventTiming time);

    EventName getName() const;

    EventTiming getTiming() const;
};


