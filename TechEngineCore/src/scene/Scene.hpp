#pragma once

#include <list>
#include "event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "event/events/gameObjects/GameObjectDestroyEvent.hpp"
#include "GameObject.hpp"

#include <boost/uuid/uuid_generators.hpp>

namespace TechEngine {
    class Scene {
    private:
        std::string name;
        boost::uuids::random_generator goTagGenerator;
        inline static Scene *instance;
        std::list<GameObject *> gameObjects;
        std::list<GameObject *> lights;

    public:

        Scene(std::string name = "default scene");

        virtual ~Scene();

        virtual void registerGameObject(GameObject *gameObject);

        virtual void unregisterGameObject(GameObject *gameObject);

        void update();

        void fixedUpdate();

        std::string genGOTag();

        static Scene &getInstance();

        std::list<GameObject *> &getGameObjects();

        std::list<GameObject *> &getLights();

        GameObject *getGameObject(const std::string &name);

        bool isLightingActive() const;

        const std::string &getName() const;

        void setName(const std::string &name);

        void clear();

        void makeChildTo(GameObject *parent, GameObject *child);
    };
}


