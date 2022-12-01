#pragma once

#include "event/events/Event.hpp"
#include "scene/GameObject.hpp"

namespace TechEngine {
    class OnDeselectGameObjectEvent : public TechEngineCore::Event {
    private:
        GameObject *gameObject;
    public:
        inline static EventType eventType = EventType("OnDeselectGameObjectEvent", ASYNC);

        OnDeselectGameObjectEvent(GameObject *gameObject) : Event(eventType) {
            this->gameObject = gameObject;
        }

        ~OnDeselectGameObjectEvent() = default;

        GameObject *getGameObject() {
            return gameObject;
        };
    };

}
