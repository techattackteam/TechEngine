#pragma once

#include <list>
#include "../events/gameObjects/GameObjectCreateEvent.hpp"
#include "../events/gameObjects/GameObjectDestroyEvent.hpp"
#include "../components/CameraComponent.hpp"
#include "GameObject.hpp"

namespace TechEngine {
    class Scene {
    public:
        inline static Scene *instance;

        Scene();

        void update();

        void fixedUpdate();

        CameraComponent *mainCamera;

        static Scene &getInstance();

        std::list<GameObject *> getGameObjects();

        std::list<GameObject *> getLights();

        bool hasMainCamera();

        bool isLightingActive() const;

    private:

        std::list<GameObject *> gameObjects;
        std::list<GameObject *> lights;

        void onGOCreate(GameObjectCreateEvent *event);

        void onGODestroy(GameObjectDestroyEvent *event);
    };
}


