#pragma once

#include <list>
#include "event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "event/events/gameObjects/GameObjectDestroyEvent.hpp"
#include "GameObject.hpp"

namespace TechEngine {
    class CoreScene {
    public:

        CoreScene();

        virtual ~CoreScene() = default;

        void update();

        void fixedUpdate();

        static CoreScene &getInstance();

        std::list<GameObject *> getGameObjects();

    protected:
        std::list<GameObject *> gameObjects;

        virtual void onGOCreate(GameObjectCreateEvent *event);

        virtual void onGODestroy(GameObjectDestroyEvent *event);

    private:
        inline static CoreScene *instance;
    };
}


