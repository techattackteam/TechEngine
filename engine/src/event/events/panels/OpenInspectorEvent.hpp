#pragma once

#include "../Event.hpp"
#include "../../../scene/GameObject.hpp"

namespace Engine {
    class OpenInspectorEvent : public Event {
    private:
        GameObject *gameObject;
    public:
        inline static EventType eventType = EventType("OpenInspectorEvent", SYNC);

        explicit OpenInspectorEvent(GameObject *gameObject) : Event(eventType) {
            this->gameObject = gameObject;
        };

        GameObject *getGameObject() {
            return gameObject;
        }
    };
}


