#pragma once
#include <PxPhysicsAPI.h>

#include "PhysicsCallback.hpp"
#include "components/physics/RigidBody.hpp"
#include "components/physics/CylinderCollider.hpp"
#include "scene/Scene.hpp"

namespace TechEngine {
    class PhysicsEngine {
    private:
        physx::PxDefaultAllocator allocator;
        physx::PxDefaultErrorCallback errorCallback;
        physx::PxDefaultCpuDispatcher* dispatcher;
        physx::PxFoundation* foundation;
        physx::PxPhysics* physics;
        physx::PxScene* pxScene;
        physx::PxMaterial* defaultMaterial;
        physx::PxPvd* pvd;
        std::unordered_map<std::string, physx::PxRigidActor*> actors;
        std::list<std::string> rigidBodiesWithoutColliders;
        bool running = false;

        Scene& scene;
        PhysicsCallback* callback;

        friend class PhysicsCallback;

        void createScene();

    public:
        explicit PhysicsEngine(Scene& scene);

        ~PhysicsEngine();

        void init();

        void start();

        void stop();

        void onFixedUpdate();

        void clear();

        void addCollider(Collider* collider);

        void addRigidBody(RigidBody* rigidBody);

        void removeCollider(const std::string& gameObjectTag, Collider* collider);

        void removeRigidBody(const std::string& gameObjectTag);

        void updateMaterial(Collider* collider);

    private:
        void updateGameObjectPositions();

        void updateActorPositions();

        physx::PxGeometry* createGeometry(TransformComponent& transform, Collider* collider);

        physx::PxGeometry* createCylinderMesh(CylinderCollider* cylinderCollider, TransformComponent& t);

        void addActor(Collider* collider, const physx::PxTransform& transform, const physx::PxGeometry* actor, const physx::PxTransform& transformOffset);

        void removeActor(const std::string& tag);

        void updateActor(Collider* collider, const physx::PxTransform& transformOffset, const physx::PxGeometry* geometry);

        std::pair<int, physx::PxShape**> getShapes(GameObject* gameObject);

        void detachCollider(Collider* collider);
    };
}
