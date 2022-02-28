#pragma once

#include <list>
#include "../event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "../event/events/gameObjects/GameObjectDestroyEvent.hpp"
#include "../components/CameraComponent.hpp"
#include "GameObject.hpp"

namespace Engine {
    class Scene {
    public:
        inline static Scene *instance;

        Scene();

        void update();

        void fixedUpdate();

        CameraComponent *mainCamera;

        static Scene &getInstance();

        std::list<GameObject *> getGameObjects();

        bool hasMainCamera();

    private:
        std::list<GameObject *> gameObjects;

        void onGOCreate(GameObjectCreateEvent *event);

        void onGODestroy(GameObjectDestroyEvent *event);
    };
}


