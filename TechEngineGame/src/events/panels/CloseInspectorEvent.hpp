#pragma once

#include <TechEngineCore.hpp>
#include "scene/GameObject.hpp"

namespace TechEngine {
    class CloseInspectorEvent : public TechEngineCore::Event {
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


