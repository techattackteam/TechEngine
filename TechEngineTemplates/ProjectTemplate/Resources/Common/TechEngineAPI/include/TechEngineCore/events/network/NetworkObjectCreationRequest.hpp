#pragma once
#include "events/Event.hpp"

namespace TechEngine {
    template<typename T>
    class NetworkObjectCreationRequest : public Event {
    public:
        T* object;
        inline static EventType eventType = EventType("NetworkObjectCreationRequest", SYNC);

        explicit NetworkObjectCreationRequest(T* object) : object(object), Event(eventType) {
        }
    };
}
