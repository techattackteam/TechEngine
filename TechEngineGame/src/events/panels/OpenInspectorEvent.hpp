#pragma once

#include "../../scene/GameObject.hpp"

namespace TechEngine {
    class OpenInspectorEvent : public TechEngineCore::Event {
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


