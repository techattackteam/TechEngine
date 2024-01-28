#pragma once

namespace TechEngine {
    class SceneManager;
}




namespace TechEngineAPI {
    class SceneManager {
    private:
        TechEngine::SceneManager *sceneManager;
    public:
        SceneManager() = default;

        ~SceneManager() = default;

        void loadScene();
    };
}
