#pragma once

#include "components/physics/Collider.hpp"
#include "components/physics/RigidBody.hpp"
#include "event/events/Event.hpp"

namespace TechEngine {
    class AddRigidBodyEvent : public Event {
    private:
        RigidBody* rigidBody;

    public:
        inline static EventType eventType = EventType("AddRigidBodyEvent", ASYNC);

        explicit AddRigidBodyEvent(RigidBody* rigidBody) : Event(eventType) {
            this->rigidBody = rigidBody;
        }

        RigidBody* getRigidBody() {
            return rigidBody;
        }
    };
}
