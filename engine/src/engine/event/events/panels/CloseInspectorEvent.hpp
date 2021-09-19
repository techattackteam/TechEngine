#pragma once

#include "../Event.hpp"
#include "../../../scene/GameObject.hpp"

namespace Engine {
    class CloseInspectorEvent : public Event {
    private:
        GameObject *gameObject;
    public:
        inline static EventType eventType = EventType("CloseInspectorEvent", SYNC);

        explicit CloseInspectorEvent(GameObject *gameObject) : Event(eventType) {
            this->gameObject = gameObject;
        };

        GameObject *getGameObject() {
            return gameObject;
        };

    };
}


