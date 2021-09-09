#pragma once

#include "../Event.hpp"
#include "../../../scene/GameObject.hpp"

namespace Engine {
    class GameObjectCreateEvent : public Event {
    private:
        GameObject *gameObject;
    public:

        inline static EventType eventType = EventType("GameObjectCreateEvent", SYNC);

        GameObjectCreateEvent(GameObject *gameObject);

        GameObject *getGameObject();
    };
}


