#pragma once

#include "system/System.hpp"
#include "components/physics/RigidBody.hpp"
#include "components/physics/CylinderCollider.hpp"
#include "scene/Scene.hpp"

#include <Jolt/Jolt.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>

#include "BPLayerInterfaceImpl.hpp"
#include "MyBodyActivationListener.hpp"
#include "MyContactListener.hpp"
#include "ObjectLayerPairFilterImpl.hpp"
#include "ObjectVsBroadPhaseLayerFilterImpl.hpp"

namespace TechEngine {
    struct CustomData {
        std::string tag;
    };

    class CORE_DLL PhysicsEngine : public System {
    private:
        bool running = false;
        friend class PhysicsCallback;
        // This is the max amount of rigid bodies that you can add to the physics system. If you try to add more you'll get an error.
        // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
        const JPH::uint cMaxBodies = 65536;

        // This determines how many mutexes to allocate to protect rigid bodies from concurrent access. Set it to 0 for the default settings.
        const JPH::uint cNumBodyMutexes = 0;

        // This is the max amount of body pairs that can be queued at any time (the broad phase will detect overlapping
        // body pairs based on their bounding boxes and will insert them into a queue for the narrowphase). If you make this buffer
        // too small the queue will fill up and the broad phase jobs will start to do narrow phase work. This is slightly less efficient.
        // Note: This value is low because this is a simple test. For a real project use something in the order of 65536.
        const JPH::uint cMaxBodyPairs = 65536;

        // This is the maximum size of the contact constraint buffer. If more contacts (collisions between bodies) are detected than this
        // number then these contacts will be ignored and bodies will start interpenetrating / fall through the world.
        // Note: This value is low because this is a simple test. For a real project use something in the order of 10240.
        const JPH::uint cMaxContactConstraints = 10240;

        SystemsRegistry& systemsRegistry;
        JPH::PhysicsSystem* physics_system;
        JPH::JobSystemThreadPool* job_system;
        JPH::TempAllocatorImpl* temp_allocator;
        /*MyBodyActivationListener body_activation_listener;
        MyContactListener contact_listener;*/
        std::vector<JPH::BodyID> bodyIDs;
        // Create mapping table from object layer to broadphase layer
        // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
        BPLayerInterfaceImpl* broad_phase_layer_interface;

        // Create class that filters object vs broadphase layers
        // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
        ObjectVsBroadPhaseLayerFilterImpl* object_vs_broadphase_layer_filter;

        // Create class that filters object vs object layers
        // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
        ObjectLayerPairFilterImpl* object_vs_object_layer_filter;

        JPH::BoxShapeSettings* floor_shape_settings;

    public:
        explicit PhysicsEngine(SystemsRegistry& systemsRegistry);

        ~PhysicsEngine() override;

        void runSimulation();

        void init();

        void start();

        void stop();

        void onFixedUpdate();

        void clear();

        void addCollider(Collider* collider);

        void addRigidBody(RigidBody* rigidBody);

        void removeCollider(const std::string& gameObjectTag, Collider* collider);

        void removeRigidBody(const std::string& gameObjectTag);

        void drawDebug(JPH::DebugRenderer& debugRenderer);

    private:
        JPH::Shape* createGeometry(TransformComponent& transform, Collider* collider);

        void updateBodies();

        void updateGameObjects();
    };
}
