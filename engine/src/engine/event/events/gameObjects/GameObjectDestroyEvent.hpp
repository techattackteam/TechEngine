#pragma once

#include "../Event.hpp"
#include "../../../scene/GameObject.hpp"

namespace Engine {
    class GameObjectDestroyEvent : public Event {
    private:
        GameObject *gameObject;
    public:
        static inline EventType eventType = EventType("GameObjectDestroyEvent", SYNC);

        GameObjectDestroyEvent(GameObject *gameObject);

        GameObject* getGameObject();
    };
}


