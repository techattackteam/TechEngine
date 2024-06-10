#pragma once
#include <PxSimulationEventCallback.h>

#include "scene/Scene.hpp"


namespace TechEngine {
    class PhysicsEngine;

    class PhysicsCallback : public physx::PxSimulationEventCallback {
    private:
        EventDispatcher& eventDispatcher;
        PhysicsEngine& physicsEngine;
        Scene& scene;

    public:
        PhysicsCallback(EventDispatcher& eventDispatcher, Scene& scene, PhysicsEngine& physicsEngine) : eventDispatcher(eventDispatcher), scene(scene), physicsEngine(physicsEngine) {
        }

        void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;

        void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;

        void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) override;

        void onWake(physx::PxActor** actors, physx::PxU32 count) override;

        void onSleep(physx::PxActor** actors, physx::PxU32 count) override;

        void onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) override;
    };
}
