#include "SceneManagerAPI.hpp"
#include "scene/SceneManager.hpp"

namespace TechEngineAPI {
    SceneManagerAPI::SceneManagerAPI(TechEngine::SceneManager* sceneManager) {
        SceneManagerAPI::sceneManager = sceneManager;
    }

    SceneManagerAPI::~SceneManagerAPI() = default;


    void SceneManagerAPI::loadScene(const std::string&sceneName) {
        sceneManager->loadScene(sceneName);
    }

    void SceneManagerAPI::saveScene(const std::string&sceneName) {
        sceneManager->saveScene(sceneName);
    }

    void SceneManagerAPI::saveCurrentScene() {
        sceneManager->saveCurrentScene();
    }

    const std::string& SceneManagerAPI::getActiveSceneName() {
        return sceneManager->getActiveSceneName();
    }

    std::shared_ptr<Scene> SceneManagerAPI::getScene() {
        return std::make_shared<Scene>(&sceneManager->getScene());
    }
}
