#pragma once

#include <list>
#include "event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "event/events/gameObjects/GameObjectDestroyEvent.hpp"
#include "GameObject.hpp"

namespace TechEngine {
    class CoreScene {
    private:
        std::string name;
    public:

        CoreScene(const std::string &name = "default scene");

        virtual ~CoreScene();

        void update();

        void fixedUpdate();

        static CoreScene &getInstance();

        std::list<GameObject *> &getGameObjects();

        GameObject *getGameObject(std::string name);

    protected:
        std::list<GameObject *> gameObjects;

        virtual void onGOCreate(GameObjectCreateEvent *event);

        virtual void onGODestroy(GameObjectDestroyEvent *event);

    private:
        inline static CoreScene *instance;
    public:
        const std::string &getName() const;

        void setName(const std::string &name);

        virtual void clear();
    };
}


