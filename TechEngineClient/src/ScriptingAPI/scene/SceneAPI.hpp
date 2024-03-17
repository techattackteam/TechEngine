#pragma once
#include <memory>
#include <string>

#include "GameObjectAPI.hpp"

namespace TechEngine {
    class Scene;
}

namespace TechEngine {
    class SceneAPI {
    private:
        Scene* scene = nullptr;

    public:
        explicit SceneAPI(Scene* scene);

        std::shared_ptr<GameObjectAPI> createGameObject(const std::string& name);

        std::shared_ptr<GameObjectAPI> getGameObject(const std::string& name);
    };
}
