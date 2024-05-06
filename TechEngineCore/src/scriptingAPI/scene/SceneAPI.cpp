#include "SceneAPI.hpp"
#include "scene/Scene.hpp"

namespace TechEngine {
    SceneAPI::SceneAPI(Scene* scene) {
        this->scene = scene;
    }

    std::shared_ptr<GameObjectAPI> SceneAPI::createGameObject(const std::string& name) {
        return std::make_shared<GameObjectAPI>(&scene->createGameObject(name));
    }

    std::shared_ptr<GameObjectAPI> SceneAPI::getGameObject(const std::string& name) {
        return std::make_shared<GameObjectAPI>(scene->getGameObject(name));
    }

    void SceneAPI::deleteGameObject(const std::string& name) {
        scene->deleteGameObject(scene->getGameObject(name));
    }

    void SceneAPI::deleteGameObject(const std::shared_ptr<GameObjectAPI>& gameObject) {
        scene->deleteGameObject(gameObject->gameObject);
    }
}
