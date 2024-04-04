#pragma once

#include "components/physics/Collider.hpp"
#include "event/events/Event.hpp"

namespace TechEngine {
    class AddColliderEvent : public Event {
    private:
        Collider* collider;

    public:
        inline static EventType eventType = EventType("AddColliderEvent", ASYNC);

        explicit AddColliderEvent(Collider* collider) : Event(eventType) {
            this->collider = collider;
        }

        Collider* getCollider() {
            return collider;
        }
    };
}
