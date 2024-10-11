#pragma once


#include <cstdarg>
#include <iostream>

#include "systems/System.hpp"
#include "Layers.hpp"
#include "BPLayerInterfaceImpl.hpp"
#include "DebugRenderer.hpp"
#include "ObjectLayerPairFilterImpl.hpp"
#include "ObjectVsBroadPhaseLayerFilterImpl.hpp"
#include "MyBodyActivationListener.hpp"
#include "MyContactListener.hpp"
#include "components/Components.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>

// Callback for traces, connect this to your own trace function if you have one
static void TraceImpl(const char* inFMT, ...) {
    // Format the message
    va_list list;
    va_start(list, inFMT);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), inFMT, list);
    va_end(list);

    // Print to the TTY
    std::cout << buffer << std::endl;
}

#ifdef JPH_ENABLE_ASSERTS

// Callback for asserts, connect this to your own assert handler if you have one
static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine) {
    // Print to the TTY
    std::cout << inFile << ":" << inLine << ": (" << inExpression << ") " << (inMessage != nullptr ? inMessage : "") << std::endl;

    // Breakpoint
    return true;
};

#endif // JPH_ENABLE_ASSERTS

namespace TechEngine {
    enum class ColliderType {
        BOX,
        SPHERE
    };

    class CORE_DLL PhysicsEngine : public System {
    private:
        JPH::PhysicsSystem* physics_system;
        JPH::TempAllocatorImpl* temp_allocator;
        JPH::JobSystemThreadPool* job_system;
        MyContactListener contact_listener;
        MyBodyActivationListener body_activation_listener;

        BPLayerInterfaceImpl broad_phase_layer_interface;
        ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
        ObjectLayerPairFilterImpl object_vs_object_layer_filter;
        int step = 0;
        SystemsRegistry& m_systemsRegistry;

        //std::unordered_map<std::string, std::vector<JPH::Body*>> m_bodies;

    public:
        DebugRenderer* debugRenderer = nullptr;

        explicit PhysicsEngine(SystemsRegistry& systemsRegistry);

        void init() override;

        void shutdown() override;

        void onUpdate() override;

        void onFixedUpdate() override;

        void renderBodies();

        const JPH::BodyID& createBody(const ::TechEngine::ColliderType& type, const ::TechEngine::Transform& transform, glm::vec3 offset, glm::vec3 size);

        void updateBodies();

        void updateEntities();

    private:
        template<typename Collider>
        void updateBodies(Transform& transform, Collider& collider) {
            JPH::BodyInterface& body_interface = physics_system->GetBodyInterface();
            JPH::RVec3 position = JPH::RVec3(transform.position.x, transform.position.y, transform.position.z);
            JPH::Quat quat = JPH::Quat(transform.rotation.x, transform.rotation.y, transform.rotation.z, 1.0f);
            body_interface.SetPositionAndRotation(collider.bodyID, position, quat, JPH::EActivation::DontActivate);
        }

        template<typename Collider>
        void updateEntities(Transform& transform, Collider& collider) {
            JPH::BodyInterface& bodyInterface = physics_system->GetBodyInterface();
            JPH::RVec3 position = bodyInterface.GetCenterOfMassPosition(collider.bodyID);
            JPH::Quat rotation = bodyInterface.GetRotation(collider.bodyID);
            transform.position = glm::vec3(position.GetX(), position.GetY(), position.GetZ());
            transform.rotation = glm::vec3(rotation.GetX(), rotation.GetY(), rotation.GetZ());
        }
    };
}
