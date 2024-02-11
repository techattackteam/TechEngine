#pragma once
#include <memory>
#include <string>

#include "Scene.hpp"

namespace TechEngine {
    class Script;
    class SceneManager;
}

namespace TechEngineAPI {
    class SceneManagerAPI {
    private:
        inline static TechEngine::SceneManager* sceneManager = nullptr;

    public:
        explicit SceneManagerAPI(TechEngine::SceneManager* sceneManager);

        ~SceneManagerAPI();

        static void loadScene(const std::string&sceneName);

        static void saveScene(const std::string&sceneName);

        static void saveCurrentScene();

        static const std::string& getActiveSceneName();

        static std::shared_ptr<Scene> getScene();
    };
}
