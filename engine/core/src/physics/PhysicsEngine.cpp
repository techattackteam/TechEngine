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

        //body_activation_listener = new MyBodyActivationListener();
        physics_system->SetBodyActivationListener(&body_activation_listener);

        // A contact listener gets notified when bodies (are about to) collide, and when they separate again.
        // Note that this is called from a job so whatever you do here needs to be thread safe.
        // Registering one is entirely optional.

        physics_system->SetContactListener(&contact_listener);

        // The main way to interact with the bodies in the physics system is through the body interface. There is a locking and a non-locking
        // variant of this. We're going to use the locking version (even though we're not planning to access bodies from multiple threads)
        JPH::BodyInterface& body_interface = physics_system->GetBodyInterface();

        // Next we can create a rigid body to serve as the floor, we make a large box
        // Create the settings for the collision volume (the shape).
        // Note that for simple shapes (like boxes) you can also directly construct a BoxShape.
        JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(20.0f, 1.0f, 20.0f));
        floor_shape_settings.SetEmbedded(); // A ref counted object on the stack (base class RefTarget) should be marked as such to prevent it from being freed when its reference count goes to 0.

        // Create the shape
        JPH::ShapeSettings::ShapeResult floor_shape_result = floor_shape_settings.Create();
        JPH::ShapeRefC floor_shape = floor_shape_result.Get(); // We don't expect an error here, but you can check floor_shape_result for HasError() / GetError()

        // Create the settings for the body itself. Note that here you can also set other properties like the restitution / friction.
        JPH::BodyCreationSettings floor_settings(floor_shape, JPH::RVec3(0.0, -5.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);

        // Create the actual rigid body
        JPH::Body* floor = body_interface.CreateBody(floor_settings); // Note that if we run out of bodies this can return nullptr

        // Add it to the world
        body_interface.AddBody(floor->GetID(), JPH::EActivation::DontActivate);

        // Now create a dynamic body to bounce on the floor
        // Note that this uses the shorthand version of creating and adding a body to the world
        JPH::BodyCreationSettings sphere_settings(new JPH::SphereShape(0.5f), JPH::RVec3(0.0, 10.0, 0.0), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
        sphere_id = body_interface.CreateAndAddBody(sphere_settings, JPH::EActivation::Activate);

        body_interface.SetLinearVelocity(sphere_id, JPH::Vec3(0.0f, 0.0f, 0.0f));

        // We simulate the physics world in discrete time steps. 60 Hz is a good rate to update the physics system.

        // Optional step: Before starting the physics simulation you can optimize the broad phase. This improves collision detection performance (it's pointless here because we only have 2 bodies).
        // You should definitely not call this every frame or when e.g. streaming in a new level section as it is an expensive operation.
        // Instead insert all new objects in batches instead of 1 at a time to keep the broad phase efficient.
        physics_system->OptimizeBroadPhase();

        // Now we're ready to simulate the body, keep simulating until it goes to sleep
        /*while (body_interface.IsActive(sphere_id)) {
            // Next step
            ++step;

            // Output current position and velocity of the sphere
            RVec3 position = body_interface.GetCenterOfMassPosition(sphere_id);
            Vec3 velocity = body_interface.GetLinearVelocity(sphere_id);
            cout << "Step " << step << ": Position = (" << position.GetX() << ", " << position.GetY() << ", " << position.GetZ() << "), Velocity = (" << velocity.GetX() << ", " << velocity.GetY() << ", " << velocity.GetZ() << ")" << endl;

            // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
            const int cCollisionSteps = 1;

            // Step the world
            physics_system->Update(cDeltaTime, cCollisionSteps, temp_allocator, job_system);
        }*/

        /*// Remove the sphere from the physics system. Note that the sphere itself keeps all of its state and can be re-added at any time.
        body_interface.RemoveBody(sphere_id);

        // Destroy the sphere. After this the sphere ID is no longer valid.
        body_interface.DestroyBody(sphere_id);

        // Remove and destroy the floor
        body_interface.RemoveBody(floor->GetID());
        body_interface.DestroyBody(floor->GetID());

        // Unregisters all types with the factory and cleans up the default material
        UnregisterTypes();

        // Destroy the factory
        delete Factory::sInstance;
        Factory::sInstance = nullptr;*/
    }

    void PhysicsEngine::shutdown() {
        System::shutdown();
        JPH::UnregisterTypes();
        delete temp_allocator;
        delete job_system;
        delete debugRenderer;
        //delete contact_listener;
        //delete body_activation_listener;
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
        delete physics_system;
    }


    void PhysicsEngine::onUpdate() {
        System::onUpdate();
        if (debugRenderer != nullptr) {
            physics_system->DrawBodies(JPH::BodyManager::DrawSettings(), debugRenderer);
        }
    }

    void PhysicsEngine::onFixedUpdate() {
        JPH::BodyInterface& body_interface = physics_system->GetBodyInterface();
        if (body_interface.IsActive(sphere_id)) {
            // Next step
            ++step;

            // Output current position and velocity of the sphere
            JPH::RVec3 position = body_interface.GetCenterOfMassPosition(sphere_id);
            JPH::Vec3 velocity = body_interface.GetLinearVelocity(sphere_id);
            //TE_LOGGER_INFO("Step {0}: Position = ({1}, {2}, {3}), Velocity = ({4}, {5}, {6})", step, position.GetX(), position.GetY(), position.GetZ(), velocity.GetX(), velocity.GetY(), velocity.GetZ());
            // If you take larger steps than 1 / 60th of a second you need to do multiple collision steps in order to keep the simulation stable. Do 1 collision step per 1 / 60th of a second (round up).
            const int cCollisionSteps = 1;

            // Step the world
            Timer& timer = m_systemsRegistry.getSystem<Timer>();
            physics_system->Update(timer.getTPS(), cCollisionSteps, temp_allocator, job_system);
        }
    }
}
