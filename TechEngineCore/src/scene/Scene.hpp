#pragma once

#include "eventSystem/EventDispatcher.hpp"
#include "GameObject.hpp"
#include "components/render/CameraComponent.hpp"
#include "events/material/MaterialUpdateEvent.hpp"

namespace TechEngine {
    class CORE_DLL Scene {
    private:
        SystemsRegistry& systemsRegistry;
        std::vector<GameObject*> gameObjects;
        std::vector<GameObject*> lights;
        std::vector<GameObject*> gameObjectsToDelete;

    public:
        Scene(SystemsRegistry& systemsRegistry);

        virtual ~Scene();

        GameObject& createGameObject(const std::string& name);

        GameObject& registerGameObject(const std::string& name, const std::string& tag);

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

        void onMaterialUpdateEvent(MaterialUpdateEvent& event);
    };
}
