#include "GameObjectDestroyEvent.hpp"

namespace Engine {

    GameObjectDestroyEvent::GameObjectDestroyEvent(GameObject *gameObject) : Event(eventType) {
        this->gameObject = gameObject;
    }

    GameObject *GameObjectDestroyEvent::getGameObject() {
        return gameObject;
    }
}