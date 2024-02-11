#pragma once
#include <memory>
#include <string>

#include "GameObject.hpp"


namespace TechEngine {
    class Scene;
}

namespace TechEngineAPI {
    class Scene {
    private:
        TechEngine::Scene* scene = nullptr;

    public:
        explicit Scene(TechEngine::Scene* scene);

        std::shared_ptr<TechEngineAPI::GameObject> createGameObject(const std::string&name);

        std::shared_ptr<TechEngineAPI::GameObject> getGameObject(const std::string&name);
    };
}
