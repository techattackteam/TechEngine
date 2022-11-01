#pragma once

#include "scene/CoreScene.hpp"
#include "components/CameraComponent.hpp"
#include "core/Core.hpp"

namespace TechEngine {
    class Scene : public CoreScene {
    private:
        inline static Scene *instance;

        std::list<GameObject *> lights;

    public:
        CameraComponent *mainCamera;

        Scene(const std::string &name = "default scene");

        ~Scene() = default;

        void onGOCreate(GameObjectCreateEvent *event) override;

        void onGODestroy(GameObjectDestroyEvent *event) override;

        bool isLightingActive() const;

        bool hasMainCamera();

        std::list<GameObject *> getLights();

        static Scene &getInstance();

        void clear();
    };
}
