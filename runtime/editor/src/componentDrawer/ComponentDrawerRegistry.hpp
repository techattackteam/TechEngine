#pragma once

#include "TechEngine/core/components/Entity.hpp"
#include "TechEngine/core/scene/Scene.hpp"

namespace TechEngine {
    struct ComponentDrawer {
        std::string displayName;
        std::function<bool(Entity, Scene&)> hasComponent;
        std::function<void(Entity, Scene&)> draw;
        std::function<void(Entity, Scene&)> onRemove;
    };

    class ComponentDrawerRegistry {
    private:
        std::vector<ComponentDrawer> m_drawers;
        SystemsRegistry& m_systemsRegistry;

    public:
        ComponentDrawerRegistry(SystemsRegistry& systemsRegistry);

        void drawAll(Entity entity, Scene& scene, std::function<void(const ComponentDrawer&, Entity, Scene&)> drawFunction) const;

        void registerAllDrawers();

    private:
        template<typename T>
        void registerDrawer(std::string name, std::function<void(T&, Entity, Scene&)> drawFunction, std::function<void(Entity, Scene&)> onRemoveFunction = nullptr) {
            ComponentDrawer drawer;
            drawer.displayName = name;
            drawer.hasComponent = [](Entity entity, Scene& scene) {
                return scene.hasComponent<T>(entity);
            };
            drawer.draw = [drawFunction](Entity entity, Scene& scene) {
                drawFunction(scene.getComponent<T>(entity), entity, scene);
            };
            drawer.onRemove = onRemoveFunction
                                  ? onRemoveFunction
                                  : [](Entity e, Scene& scene) {
                                      scene.removeComponent<T>(e);
                                  };

            m_drawers.push_back(std::move(drawer));
        }

        void registerTransform();

        void registerCamera();

        void registerMeshRenderer();

        void registerPointLight();

        void registerDirectionalLight();

        void registerSpotLight();

        void registerStaticBody();

        void registerKinematicBody();

        void registerRigidBody();

        void registerBoxCollider();

        void registerSphereCollider();

        void registerCapsuleCollider();

        void registerCylinderCollider();

        void registerBoxTrigger();

        void registerSphereTrigger();

        void registerCapsuleTrigger();

        void registerCylinderTrigger();

        void registerAudioListener();

        void registerAudioEmitter();
    };
}
