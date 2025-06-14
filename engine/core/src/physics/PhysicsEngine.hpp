#pragma once


#include <cstdarg>
#include <iostream>


#include "ShapeFactory.hpp"
#include "BPLayerInterfaceImpl.hpp"
//#include "DebugRenderer.hpp"
#include "ObjectLayerPairFilterImpl.hpp"
#include "ObjectVsBroadPhaseLayerFilterImpl.hpp"
#include "MyContactListener.hpp"
#include "components/Components.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>


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
    class CORE_DLL PhysicsEngine : public System {
        friend class MyContactListener;

    private:
        SystemsRegistry& m_systemsRegistry;
        JPH::PhysicsSystem* m_physicsSystem;
        JPH::TempAllocatorImpl* temp_allocator;
        JPH::JobSystemThreadPool* job_system;
        //MyBodyActivationListener body_activation_listener;

        BPLayerInterfaceImpl broad_phase_layer_interface;
        ObjectVsBroadPhaseLayerFilterImpl object_vs_broadphase_layer_filter;
        ObjectLayerPairFilterImpl object_vs_object_layer_filter;
        MyContactListener contact_listener;
        bool m_running = false;

        std::unordered_map<std::string, JPH::BodyID> m_bodies;
        std::unordered_map<std::string, JPH::BodyID> m_triggers;
        std::unordered_map<std::string, JPH::Ref<JPH::MutableCompoundShape>> m_colliders;

    public:
        //DebugRenderer* debugRenderer = nullptr;

        explicit PhysicsEngine(SystemsRegistry& systemsRegistry);

        void init() override;

        void shutdown() override;

        bool start();

        void stop();

        void onUpdate() override;

        void onFixedUpdate() override;

        void renderBodies() const;

        const JPH::BodyID& createStaticBody(const Tag& tag, const Transform& transform);

        const JPH::BodyID& createKinematicBody(const Tag& tag, const Transform& transform);

        const JPH::BodyID& createRigidBody(const Tag& tag, const Transform& transform);

        void createCollider(Shape shape, const Tag& tag, const Transform& transform, glm::vec3 center, glm::vec3 size);

        const JPH::BodyID& createTrigger(Shape shape, const Tag& tag, const Transform& transform, glm::vec3 center,
                                         glm::vec3 size);

        void resizeCollider(Shape shape, const Tag& tag, const Transform& transform, glm::vec3 center, glm::vec3 size);

        void resizeTrigger(Shape shape, const Tag& tag, const Transform& transform, glm::vec3 center, glm::vec3 size);

        void recenterCollider(const Tag& tag, glm::vec3 center);

        void recenterTrigger(const Tag& tag, glm::vec3 center);

        void rescaleCollider(const Tag& tag, const Transform& transform, glm::vec3 center, bool uniform = false);

        void rescaleTrigger(const Tag& tag, const Transform& transform, glm::vec3 center, bool uniform = false);

        void removeBody(const Tag& tag);

        void removeCollider(const Tag& tag);

        void removeTrigger(const Tag& tag);

        void setVelocity(const Tag& tag, const glm::vec3& velocity);

        //Only to be used by the editor when then simulation is not running
        void moveOrRotateBody(const Tag& tag, const Transform& transform);

        void addVelocity(const Tag& tag, const glm::vec3& velocity);

        void setLinearVelocity(const Tag& tag, const glm::vec3& velocity);

    private:
        const JPH::BodyID& createBody(JPH::EMotionType eMotionType, const Tag& tag, const Transform& transform,
                                      bool isTrigger);

        template<typename Body>
        void updateBodies(Transform& transform, Body& body) {
            JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
            JPH::RVec3 position = JPH::RVec3(transform.m_position.x, transform.m_position.y, transform.m_position.z);
            glm::quat rotation = glm::quat(glm::radians(transform.m_rotation));
            JPH::Quat quat = JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w);
            bodyInterface.SetPositionAndRotation(body.bodyID, position, quat, JPH::EActivation::Activate);
        }

        template<typename Body>
        void updateEntities(Transform& transform, Body& body) {
            JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
            JPH::RVec3 position = bodyInterface.GetPosition(body.bodyID);
            JPH::Quat rotation = bodyInterface.GetRotation(body.bodyID);
            glm::vec3 euler = glm::eulerAngles(glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(),
                                                         rotation.GetZ()));

            transform.m_position = glm::vec3(position.GetX(), position.GetY(), position.GetZ());
            transform.m_rotation = glm::vec3(glm::degrees(euler.x), glm::degrees(euler.y), glm::degrees(euler.z));
        }

        void updateBodies();

        void updateEntities();
    };
}
