#pragma once

#include <list>
#include "event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "event/events/gameObjects/GameObjectDestroyEvent.hpp"
#include "GameObject.hpp"

#include <boost/uuid/uuid_generators.hpp>

namespace TechEngine {
    class CoreScene {
    private:
        std::string name;
        boost::uuids::random_generator goTagGenerator;
        inline static CoreScene *instance;
    protected:
        std::list<GameObject *> gameObjects;

        virtual void onGOCreate(GameObjectCreateEvent *event);

        virtual void onGODestroy(GameObjectDestroyEvent *event);

    public:

        CoreScene(const std::string &name = "default scene");

        virtual ~CoreScene();

        void update();

        void fixedUpdate();

        std::string genGOTag();

        static CoreScene &getInstance();

        std::list<GameObject *> &getGameObjects();

        GameObject *getGameObject(std::string name);

        const std::string &getName() const;

        void setName(const std::string &name);

        virtual void clear();
    };
}


