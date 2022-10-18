#pragma once

#include "event/events/Event.hpp"
#include "scene/GameObject.hpp"
#include "core/Core.hpp"

namespace TechEngine {
    class GameObjectDestroyEvent : public TechEngineCore::Event {
    private:
        GameObject *gameObject;
    public:
        Engine_API static inline EventType eventType = EventType("GameObjectDestroyEvent", SYNC);

        explicit GameObjectDestroyEvent(GameObject *gameObject) : Event(eventType) {
            this->gameObject = gameObject;
        }

        GameObject *getGameObject() {
            return gameObject;
        }
    };
}


