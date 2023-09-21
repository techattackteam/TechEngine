#pragma once

#include <PxPhysicsAPI.h>
#include "components/physics/BoxColliderComponent.hpp"
#include "components/physics/SphereCollider.hpp"

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

        void addCollider(Collider *collider);

        void removeCollider(const std::string& gameObjectTag,Collider *collider);

        void updateGameObjectPositions();

        void updateActorPositions();

    private:
        void addActor(Collider *collider, physx::PxTransform transform, const physx::PxGeometry &actor);

        void removeActor(const std::string &tag);
    };
}
