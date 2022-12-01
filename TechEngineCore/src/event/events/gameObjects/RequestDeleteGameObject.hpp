#pragma once

#include "event/events/Event.hpp"
#include "scene/GameObject.hpp"

namespace TechEngine {
    class RequestDeleteGameObject : public TechEngineCore::Event {
    private:
        GameObject *gameObject;
    public:
        static inline EventType eventType = EventType("RequestDeleteGameObject", SYNC);

        explicit RequestDeleteGameObject(GameObject *gameObject) : Event(eventType) {
            this->gameObject = gameObject;
        }

        GameObject *getGameObject() {
            return gameObject;
        }
    };
}


