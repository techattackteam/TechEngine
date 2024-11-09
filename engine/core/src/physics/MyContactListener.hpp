#pragma once

#include "systems/SystemsRegistry.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>

namespace JPH {
    class PhysicsSystem;
}

namespace TechEngine {
    class MyContactListener : public JPH::ContactListener {
    private:
        std::unordered_map<std::string, JPH::BodyID>* m_bodies = nullptr;
        std::unordered_map<std::string, JPH::BodyID>* m_triggers = nullptr;

        JPH::PhysicsSystem* m_physicsSystem = nullptr;
        SystemsRegistry& m_systemsRegistry;

    public:
        explicit MyContactListener(SystemsRegistry& systemsRegistry);

        void init(std::unordered_map<std::string, JPH::BodyID>* bodies,
                  std::unordered_map<std::string, JPH::BodyID>* triggers,
                  JPH::PhysicsSystem* physicsSystem);

        // See: ContactListener
        JPH::ValidateResult OnContactValidate(const JPH::Body& inBody1, const JPH::Body& inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult& inCollisionResult) override;

        void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;

        void OnContactPersisted(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings) override;

        void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;

        bool isTrigger(const JPH::Body& body);
    };
}
