#pragma once

#include "event/events/Event.hpp"
#include "scene/GameObject.hpp"

namespace TechEngine {
    class OnSelectGameObjectEvent : public TechEngineCore::Event {
    private:
        GameObject *gameObject;
    public:
        inline static EventType eventType = EventType("OnSelectGameObject", ASYNC);

        OnSelectGameObjectEvent(GameObject *gameObject) : Event(eventType) {
            this->gameObject = gameObject;
        }

        ~OnSelectGameObjectEvent() = default;

        GameObject *getGameObject() {
            return gameObject;
        };
    };

}
