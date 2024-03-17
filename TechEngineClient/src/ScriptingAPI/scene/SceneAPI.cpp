#include "SceneAPI.hpp"
#include "scene/Scene.hpp"

namespace TechEngine {
    SceneAPI::SceneAPI(Scene* scene) {
        this->scene = scene;
    }

    std::shared_ptr<GameObjectAPI> SceneAPI::createGameObject(const std::string& name) {
        return std::make_shared<GameObjectAPI>(&scene->createGameObject<GameObject>(name));
    }

    std::shared_ptr<GameObjectAPI> SceneAPI::getGameObject(const std::string& name) {
        return std::make_shared<GameObjectAPI>(scene->getGameObject(name));
    }
}
