#pragma once

#include <string>
#include "core/CoreExportDll.hpp"
using EventName = std::string;

enum EventTiming {
    SYNC, FIXED_SYNC, ASYNC, LAZY_ASYNC
};

class CORE_DLL EventType {
private:
    EventName name;
    EventTiming timing;

public:
    EventType(std::string name, EventTiming time);

    EventName getName() const;

    EventTiming getTiming() const;
};
