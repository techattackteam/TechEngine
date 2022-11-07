#pragma once

#include "event/events/Event.hpp"
#include "scene/GameObject.hpp"

namespace TechEngine {
    class GameObjectDestroyEvent : public TechEngineCore::Event {
    private:
        GameObject *gameObject;
    public:
        static inline EventType eventType = EventType("GameObjectDestroyEvent", ASYNC);

        explicit GameObjectDestroyEvent(GameObject *gameObject) : Event(eventType) {
            this->gameObject = gameObject;
        }

        GameObject *getGameObject() {
            return gameObject;
        }
    };
}


