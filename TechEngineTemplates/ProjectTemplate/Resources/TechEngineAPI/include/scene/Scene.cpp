#include "Scene.hpp"

#include "scene/Scene.hpp"

#include "components/MeshRendererComponent.hpp"

namespace TechEngineAPI {
    Scene::Scene(TechEngine::Scene* scene) {
        this->scene = scene;
    }

    std::shared_ptr<TechEngineAPI::GameObject> Scene::createGameObject(const std::string& name) {
        return std::make_shared<TechEngineAPI::GameObject>(&scene->createGameObject<TechEngine::GameObject>(name));
    }

    std::shared_ptr<TechEngineAPI::GameObject> Scene::getGameObject(const std::string& name) {
        return std::make_shared<TechEngineAPI::GameObject>(scene->getGameObject(name));
    }
}
