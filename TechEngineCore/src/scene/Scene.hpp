#pragma once

#include <list>
#include "GameObject.hpp"
#include "event/events/gameObjects/GameObjectCreateEvent.hpp"
#include "event/events/gameObjects/GameObjectDestroyEvent.hpp"
#include "event/events/gameObjects/RequestDeleteGameObject.hpp"

#include <boost/uuid/uuid_generators.hpp>

namespace TechEngine {
    class Scene {
    private:
        boost::uuids::random_generator goTagGenerator;
        inline static Scene *instance;
        std::list<GameObject *> gameObjects;
        std::list<GameObject *> lights;
        std::list<GameObject *> gameObjectsToDelete;
    public:

        Scene();

        virtual ~Scene();

        virtual void registerGameObject(GameObject *gameObject);

        virtual void registerGameObject(GameObject *gameObject, std::string tag);

        virtual void unregisterGameObject(GameObject *gameObject);

        void update();

        void fixedUpdate();

        std::string genGOTag();

        static Scene &getInstance();

        std::list<GameObject *> &getGameObjects();

        std::list<GameObject *> getAllGameObjects();

        std::list<GameObject *> &getLights();

        GameObject *getGameObject(const std::string &name);

        GameObject *getGameObjectByTag(const std::string &tag);

        bool isLightingActive() const;

        void clear();

        void makeChildTo(GameObject *parent, GameObject *child);

        void onGameObjectDeleteRequest(TechEngine::RequestDeleteGameObject *event);

    };
}


