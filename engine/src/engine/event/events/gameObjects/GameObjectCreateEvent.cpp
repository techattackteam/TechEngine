#include "GameObjectCreateEvent.hpp"

namespace Engine {
    GameObjectCreateEvent::GameObjectCreateEvent(GameObject *gameObject) : Event(eventType) {
        this->gameObject = gameObject;
    }

    GameObject *GameObjectCreateEvent::getGameObject() {
        return gameObject;
    }
}
