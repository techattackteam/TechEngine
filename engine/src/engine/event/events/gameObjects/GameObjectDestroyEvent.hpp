#pragma once

#include "../Event.hpp"
#include "../../../scene/GameObject.hpp"

namespace Engine {
    class GameObjectDestroyEvent : public Event {
    private:
        GameObject *gameObject;
    public:
        static inline EventType eventType = EventType("GameObjectDestroyEvent", SYNC);

        explicit GameObjectDestroyEvent(GameObject *gameObject) : Event(eventType) {
            this->gameObject = gameObject;
        }

        GameObject *getGameObject() {
            return gameObject;
        }
    };
}


