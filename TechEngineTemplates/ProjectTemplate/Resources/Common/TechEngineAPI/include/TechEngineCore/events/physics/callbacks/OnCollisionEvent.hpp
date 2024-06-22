#pragma once

#include <vector>

#include "events/Event.hpp"

namespace TechEngine {
    class GameObjectAPI;

    class OnCollisionEvent : public Event {
    private:
        std::vector<std::shared_ptr<GameObjectAPI>> gameObjects;

    public:
        inline static EventType eventType = EventType("OnCollisonEvent", SYNC);

        explicit OnCollisionEvent(std::vector<std::shared_ptr<GameObjectAPI>> gameObjects) : Event(eventType) {
            this->gameObjects = gameObjects;
        }

        const std::vector<std::shared_ptr<GameObjectAPI>>& getGameObjects() {
            return gameObjects;
        }
    };
}
