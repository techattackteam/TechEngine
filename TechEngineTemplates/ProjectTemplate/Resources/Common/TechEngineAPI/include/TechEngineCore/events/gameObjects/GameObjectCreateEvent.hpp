#pragma once

#include "events/Event.hpp"
#include "scene/GameObject.hpp"

namespace TechEngine {
    class GameObjectCreateEvent : public Event {
    private:
        GameObject* gameObject;

    public:
        inline static EventType eventType = EventType("GameObjectCreateEvent", SYNC);

        explicit GameObjectCreateEvent(GameObject* gameObject) : Event(eventType) {
            this->gameObject = gameObject;
        }

        GameObject* getGameObject() {
            return gameObject;
        }
    };
}
