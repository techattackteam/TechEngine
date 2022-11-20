#pragma once

#include "scene/CoreScene.hpp"
#include "components/CameraComponent.hpp"

namespace TechEngine {
    class Scene : public CoreScene {
    private:
        inline static Scene *instance;

        std::list<GameObject *> lights;

    public:

        CameraComponent *mainCamera = nullptr;

        Scene(const std::string &name = "default scene");

        ~Scene() = default;

        void registerGameObject(GameObject *gameObject) override;

        void unregisterGameObject(GameObject *event) override;

        bool isLightingActive() const;

        bool findCameraComponent();

        bool hasMainCamera();

        std::list<GameObject *> getLights();

        static Scene &getInstance();

        void clear() override;
    };
}
