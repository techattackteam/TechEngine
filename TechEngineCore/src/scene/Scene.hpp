#pragma once

#include <components/render/CameraComponent.hpp>

#include "GameObject.hpp"

namespace TechEngine {
    class Scene {
    private:
        std::vector<GameObject*> gameObjects;
        std::vector<GameObject*> lights;
        std::vector<GameObject*> gameObjectsToDelete;

    public:
        Scene();

        virtual ~Scene();

        template<typename T, typename... A>
        GameObject& createGameObject(A&&... args) {
            GameObject* gameObject = new T(args...); //-> implies name already defined
            gameObject->setTag(genGOTag());
            gameObjects.push_back(gameObject);
            return *gameObject;
        }

        template<typename T, typename... A>
        GameObject& createGameObject(std::string name, A&&... args) {
            return registerGameObject<T, A...>(name, genGOTag(), args...);
        }

        template<typename T, typename... A>
        GameObject& registerGameObject(std::string name, std::string tag, A&&... args) {
            GameObject* gameObject = new T(name, tag, args...);
            gameObjects.push_back(gameObject);
            return *gameObject;
        }

        void duplicateGameObject(GameObject* game_object);

        void deleteGameObject(GameObject* gameObject);

        void update();

        void fixedUpdate();

        std::string genGOTag();

        std::vector<GameObject*> getGameObjects();

        std::vector<GameObject*> getLights();

        GameObject* getGameObject(const std::string& name);

        GameObject* getGameObjectByTag(const std::string& tag);

        bool isLightingActive() const;

        void clear();

        void makeChildTo(GameObject* parent, GameObject* child);

        void onGameObjectDeleteRequest();

        bool hasMainCamera();

        CameraComponent* getMainCamera();
    };
}
