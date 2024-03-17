#pragma once

#include "event/events/Event.hpp"
#include "scene/GameObject.hpp"

namespace TechEngine {
    class RequestDeleteGameObject : public TechEngine::Event {
    private:
        std::string tag;
    public:
        static inline EventType eventType = EventType("RequestDeleteGameObject", SYNC);

        explicit RequestDeleteGameObject(const std::string &tag) : Event(eventType) {
            this->tag = tag;
        }

        std::string &getTag() {
            return tag;
        }
    };
}


