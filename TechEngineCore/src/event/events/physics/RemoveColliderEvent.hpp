#pragma once

#include "components/physics/Collider.hpp"
#include "event/events/Event.hpp"

namespace TechEngine {
    class RemoveColliderEvent : public Event {
    private:
        Collider* collider;
        const std::string& tag;

    public:
        inline static EventType eventType = EventType("RemoveColliderEvent", ASYNC);

        explicit RemoveColliderEvent(const std::string& tag, Collider* collider) : tag(tag), Event(eventType) {
            this->collider = collider;
        }

        Collider* getCollider() {
            return collider;
        }

        const std::string& getTag() {
            return tag;
        }
    };
}
