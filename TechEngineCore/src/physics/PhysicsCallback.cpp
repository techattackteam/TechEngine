#include "PhysicsCallback.hpp"
#include "PhysicsEngine.hpp"
#include "core/Logger.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "events/physics/callbacks/OnCollisionEvent.hpp"

namespace TechEngine {
    /*void PhysicsCallback::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) {
        std::vector<std::shared_ptr<GameObjectAPI>> gameObjects = std::vector<std::shared_ptr<GameObjectAPI>>();
        for (const auto& actor: physicsEngine.actors) {
            auto it = std::find_if(std::begin(pairHeader.actors), std::end(pairHeader.actors), [&](const auto& element) {
                return actor.second == element;
            });
            if (it != std::end(pairHeader.actors)) {
                GameObject* gameObject = scene.getGameObjectByTag(actor.first);
                if (gameObject == nullptr) { // If the game object is not found, most likely it was destroyed
                    return;
                }
                //gameObjects.push_back(std::make_shared<GameObjectAPI>(gameObject));
            }
        }
        eventDispatcher.dispatch(new OnCollisionEvent(gameObjects));
    }

    void PhysicsCallback::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) {
        TE_LOGGER_INFO("Collision between {0} and {1}", pairs->triggerActor->getName(), pairs->otherActor->getName());
    }

    void PhysicsCallback::onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) {
        TE_LOGGER_INFO("Constraint break");
    }

    void PhysicsCallback::onWake(physx::PxActor** actors, physx::PxU32 count) {
        TE_LOGGER_INFO("Wake");
    }

    void PhysicsCallback::onSleep(physx::PxActor** actors, physx::PxU32 count) {
        TE_LOGGER_INFO("Sleep");
    }

    void PhysicsCallback::onAdvance(const physx::PxRigidBody* const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) {
        TE_LOGGER_INFO("Advance");
    }*/
}
