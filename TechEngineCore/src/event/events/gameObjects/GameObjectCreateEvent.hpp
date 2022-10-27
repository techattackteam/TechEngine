#pragma once

#include "event/events/Event.hpp"
#include "scene/GameObject.hpp"
#include "core/Core.hpp"
namespace TechEngine {
    class /*Engine_API*/ GameObjectCreateEvent : public TechEngineCore::Event {
    private:
        GameObject *gameObject;
    public:

        inline static EventType eventType = EventType("GameObjectCreateEvent", ASYNC);

        explicit GameObjectCreateEvent(GameObject *gameObject) : Event(eventType) {
            this->gameObject = gameObject;
        }

        GameObject *getGameObject() {
            return gameObject;
        }
    };
}


