#pragma once

#include "events/Event.hpp"

namespace TechEngine {
    class OnProcessCloseEvent : public Event {
    private:
        DWORD dwProcessId;

    public:
        inline static EventType eventType = EventType("OnProcessCloseEvent", EventTiming::ASYNC);

        OnProcessCloseEvent(DWORD dwProcessId) : dwProcessId(dwProcessId), Event(eventType) {
        }

        inline DWORD& getProcessId() {
            return dwProcessId;
        }
    };
}
