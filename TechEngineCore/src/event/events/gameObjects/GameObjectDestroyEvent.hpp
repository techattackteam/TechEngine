#pragma once

#include "event/events/Event.hpp"
#include "scene/GameObject.hpp"

namespace TechEngine {
    class GameObjectDestroyEvent : public TechEngine::Event {
    private:
        std::string tag;
    public:
        static inline EventType eventType = EventType("GameObjectDestroyEvent", SYNC);

        explicit GameObjectDestroyEvent(std::string &gameObjectTag) : Event(eventType) {
            this->tag = gameObjectTag;
        }

        std::string &getGameObjectTag() {
            return tag;
        }
    };
}


