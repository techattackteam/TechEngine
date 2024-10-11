#include "PhysicsEngine.hpp"
#include <Jolt/Jolt.h>


#include "core/Logger.hpp"
#include "core/Timer.hpp"
#include "systems/SystemsRegistry.hpp"
#include "physics/DebugRenderer.hpp"

#include <iostream>
#include <cstdarg>
#include <thread>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "components/Archetype.hpp"
#include "scene/ScenesManager.hpp"

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS


namespace TechEngine {
    PhysicsEngine::PhysicsEngine(SystemsRegistry& systemsRegistry) : m_systemsRegistry(systemsRegistry), System() {
    }

    void PhysicsEngine::init() {
        JPH::RegisterDefaultAllocator();
        JPH::Trace = TraceImpl;
        JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

        JPH::Factory::sInstance = new JPH::Factory();

        JPH::RegisterTypes();
        debugRenderer = new DebugRenderer();

        temp_allocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

        job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, JPH::thread::hardware_concurrency() - 1);

        const JPH::uint cMaxBodies = 1024;
        const JPH::uint cNumBodyMutexes = 0;
        const JPH::uint cMaxBodyPairs = 1024;
        const JPH::uint cMaxContactConstraints = 1024;


        physics_system = new JPH::PhysicsSystem();
        physics_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);

        // A body activation listener gets notified when bodies activate and go to sleep
        // Note that this is called from a job so whatever you do here needs to be thread safe.
        // Registering one is entirely optional.
        physics_system->SetBodyActivationListener(&body_activation_listener);

        // A contact listener gets notified when bodies (are about to) collide, and when they separate again.
        // Note that this is called from a job so whatever you do here needs to be thread safe.
        // Registering one is entirely optional.
        physics_system->SetContactListener(&contact_listener);

        // The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
        // variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
        JPH::BodyInterface& body_interface = physics_system->GetBodyInterface();

        // Now create a dynamic body to bounce on the floor
        // Note that this uses the shorthand version of creating and adding a body to the world
        JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(0.5f), JPH::RVec3(0.0, 10.0, 0.0),
                                                  JPH::Quat::sIdentity(),
                                                  JPH::EMotionType::Dynamic,
                                                  Layers::MOVING);
        sphere_id = body_interface.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);

        body_interface.SetLinearVelocity(sphere_id, JPH::Vec3(0.0f, 0.0f, 0.0f));

        //createBody();
        // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
        // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
        // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
        physics_system->OptimizeBroadPhase();
        TE_LOGGER_INFO("Physics engine initialized");
    }

    void PhysicsEngine::shutdown() {
        System::shutdown();
        JPH::UnregisterTypes();
        delete temp_allocator;
        delete job_system;
        delete debugRenderer;
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
        delete physics_system;
        TE_LOGGER_INFO("Physics engine shutdown");
    }


    void PhysicsEngine::onUpdate() {
    }

    void PhysicsEngine::onFixedUpdate() {
        const int cCollisionSteps = 1;
        updateBodies();
        Timer& timer = m_systemsRegistry.getSystem<Timer>();
        physics_system->Update(timer.getTPS(), cCollisionSteps, temp_allocator, job_system);
        updateEntities();
    }

    void PhysicsEngine::renderBodies() {
        if (debugRenderer != nullptr) {
            physics_system->DrawBodies(JPH::BodyManager::DrawSettings(), debugRenderer);
        }
    }

    const ::JPH::BodyID& PhysicsEngine::createBody(const Tag& tag, const Transform& transform, glm::vec3 offset, glm::vec3 size) {
        JPH::RegisterDefaultAllocator();
        JPH::BodyInterface& body_interface = physics_system->GetBodyInterface();
        glm::vec3 transformPosition = transform.position;
        glm::vec3 transformRotation = transform.rotation;
        glm::vec3 transformScale = transform.scale;

        JPH::RVec3 position = JPH::RVec3(transformPosition.x + offset.x,
                                         transformPosition.y + offset.y,
                                         transformPosition.z + offset.z);
        glm::quat rotation = glm::quat(glm::radians(transformRotation));
        JPH::Quat quat = JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w);
        JPH::RVec3 scale = JPH::RVec3(size.x * transformScale.x, size.y * transformScale.y, size.z * transformScale.z);

        JPH::BoxShape* box_shape = new JPH::BoxShape(scale);
        JPH::BodyCreationSettings floor_settings(box_shape,
                                                 position,
                                                 quat,
                                                 JPH::EMotionType::Dynamic,
                                                 Layers::MOVING);

        // Create the actual rigid body
        JPH::Body* floor = body_interface.CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr
        body_interface.AddBody(floor->GetID(), JPH::EActivation::Activate);

        return floor->GetID();
    }

    void PhysicsEngine::updateBodies() {
        m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().runSystem<Tag, Transform, BoxCollider>([this](Tag& tag, Transform& transform, BoxCollider& boxCollider) {
            JPH::BodyInterface& body_interface = physics_system->GetBodyInterface();
            JPH::RVec3 position = JPH::RVec3(transform.position.x, transform.position.y, transform.position.z);
            JPH::Quat quat = JPH::Quat(transform.rotation.x, transform.rotation.y, transform.rotation.z, 1.0f);
            body_interface.SetPositionAndRotation(boxCollider.bodyID, position, quat, JPH::EActivation::DontActivate);
        });
    }

    void PhysicsEngine::updateEntities() {
        m_systemsRegistry.getSystem<ScenesManager>().getActiveScene().runSystem<Tag, Transform, BoxCollider>([this](Tag& tag, Transform& transform, BoxCollider& boxCollider) {
            JPH::BodyInterface& body_interface = physics_system->GetBodyInterface();
            JPH::RVec3 position = body_interface.GetCenterOfMassPosition(boxCollider.bodyID);
            transform.position = glm::vec3(position.GetX(), position.GetY(), position.GetZ());
        });
    }
}
