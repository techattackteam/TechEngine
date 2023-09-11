#pragma once

#include <PxPhysicsAPI.h>
#include "components/BoxColliderComponent.hpp"

namespace TechEngine {
    class PhysicsEngine {
    private:
        inline static PhysicsEngine *instance;

        physx::PxDefaultAllocator allocator;
        physx::PxDefaultErrorCallback errorCallback;
        physx::PxDefaultCpuDispatcher *dispatcher;
        physx::PxFoundation *foundation;
        physx::PxPhysics *physics;
        physx::PxScene *scene;
        physx::PxMaterial *defaultMaterial;
        physx::PxPvd *pvd;

        std::unordered_map<std::string, physx::PxRigidActor *> actors;

        bool running = false;

        void createScene();


    public:
        static PhysicsEngine *getInstance();

        PhysicsEngine();

        ~PhysicsEngine();

        void init();

        void start();

        void stop();

        void onUpdate();

        void addBoxCollider(BoxColliderComponent *boxColliderComponent);

        void updateGameObjectPositions();

        void updateActorPositions();
    };
}
