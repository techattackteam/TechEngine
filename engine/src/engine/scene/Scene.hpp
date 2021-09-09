#pragma once

#include <list>
#include "../event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "../event/events/gameObjects/GameObjectDestroyEvent.hpp"
#include "GameObject.hpp"

namespace Engine {
    class Scene {
    public:
        Scene();

        void onGOCreate(GameObjectCreateEvent *event);


    private:
        std::list<GameObject *> gameObjects;

        void onGODestroy(GameObjectDestroyEvent *event);
    };
}


