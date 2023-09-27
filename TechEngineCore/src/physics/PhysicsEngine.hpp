#pragma once

#include <PxPhysicsAPI.h>
#include "components/physics/BoxColliderComponent.hpp"
#include "components/physics/SphereCollider.hpp"
#include "components/physics/CylinderCollider.hpp"
#include "components/physics/RigidBody.hpp"

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
        std::list<std::string> rigidBodiesWithoutColliders;
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

        void addRigidBody(RigidBody *rigidBody);

        void removeCollider(const std::string &gameObjectTag, Collider *collider);

        void removeRigidBody(const std::string &gameObjectTag);

        void updateMaterial(Collider *collider);

    private:

        void updateGameObjectPositions();

        void updateActorPositions();

        physx::PxGeometry *createGeometry(TransformComponent &transform, Collider *collider);

        physx::PxGeometry *createCylinderMesh(TechEngine::CylinderCollider *cylinderCollider, TransformComponent &t);

        void addActor(Collider *collider, const physx::PxTransform &transform, const physx::PxGeometry *actor, const physx::PxTransform &transformOffset);

        void removeActor(const std::string &tag);

        void updateActor(Collider *collider, const physx::PxTransform &transformOffset, const physx::PxGeometry *geometry);

        std::pair<int, physx::PxShape **> getShapes(GameObject *gameObject);

        void detachCollider(Collider *collider);
    };
}
