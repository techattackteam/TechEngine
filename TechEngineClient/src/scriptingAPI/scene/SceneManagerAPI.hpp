#pragma once
#include <memory>
#include <string>

#include "SceneAPI.hpp"


namespace TechEngine {
    class SceneManager;
}

namespace TechEngine {
    class SceneManagerAPI {
    private:
        inline static SceneManager* sceneManager = nullptr;

    public:
        explicit SceneManagerAPI(SceneManager* sceneManager);

        ~SceneManagerAPI();

        static void loadScene(const std::string& sceneName);

        static void saveScene(const std::string& sceneName);

        static void saveCurrentScene();

        static const std::string& getActiveSceneName();

        static std::shared_ptr<SceneAPI> getScene();
    };
}
