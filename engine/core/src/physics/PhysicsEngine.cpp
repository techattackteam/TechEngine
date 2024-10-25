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
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>

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


        m_physicsSystem = new JPH::PhysicsSystem();
        m_physicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_vs_object_layer_filter);

        // A body activation listener gets notified when bodies activate and go to sleep
        // Note that this is called from a job so whatever you do here needs to be thread safe.
        // Registering one is entirely optional.
        m_physicsSystem->SetBodyActivationListener(&body_activation_listener);

        // A contact listener gets notified when bodies (are about to) collide, and when they separate again.
        // Note that this is called from a job so whatever you do here needs to be thread safe.
        // Registering one is entirely optional.
        m_physicsSystem->SetContactListener(&contact_listener);


        TE_LOGGER_INFO("Physics engine initialized");
    }

    void PhysicsEngine::shutdown() {
        stop();
        JPH::UnregisterTypes();
        delete temp_allocator;
        delete job_system;
        delete debugRenderer;
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
        delete m_physicsSystem;
        TE_LOGGER_INFO("Physics engine shutdown");
    }

    bool PhysicsEngine::start() {
        JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
        m_running = true;
        for (auto& [uuid, body]: m_bodies) {
            if (m_colliders.find(uuid) == m_colliders.end() && m_triggers.find(uuid) == m_triggers.end()) {
                TE_LOGGER_ERROR("Body with uuid: {0} has no collider or trigger", uuid.c_str());
                return false;
            }

            bodyInterface.SetShape(body, m_colliders[uuid], true, JPH::EActivation::DontActivate);
            bodyInterface.AddBody(body, JPH::EActivation::Activate);
        }
        return true;
    }

    void PhysicsEngine::stop() {
        JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
        m_bodies.clear();
        m_colliders.clear();
        m_running = false;
    }


    void PhysicsEngine::onUpdate() {
    }

    void PhysicsEngine::onFixedUpdate() {
        const int cCollisionSteps = 1;
        updateBodies();
        Timer& timer = m_systemsRegistry.getSystem<Timer>();
        m_physicsSystem->Update(timer.getTPS(), cCollisionSteps, temp_allocator, job_system);
        updateEntities();
    }

    void PhysicsEngine::renderBodies() {
        if (debugRenderer != nullptr) {
            m_physicsSystem->DrawBodies(JPH::BodyManager::DrawSettings(), debugRenderer);
        }
    }

    const JPH::BodyID& PhysicsEngine::createStaticBody(const Tag& tag, const Transform& transform) {
        return createBody(JPH::EMotionType::Static, tag, transform);
    }

    const JPH::BodyID& PhysicsEngine::createKinematicBody(const Tag& tag, const Transform& transform) {
        return createBody(JPH::EMotionType::Kinematic, tag, transform);
    }

    const JPH::BodyID& PhysicsEngine::createRigidBody(const Tag& tag, const Transform& transform) {
        return createBody(JPH::EMotionType::Dynamic, tag, transform);
    }

    const void PhysicsEngine::createBoxCollider(const Tag& tag, const Transform& transform, glm::vec3 center, glm::vec3 scale) {
        JPH::RegisterDefaultAllocator();
        JPH::RVec3 size = JPH::RVec3(transform.scale.x, transform.scale.y, transform.scale.z);
        JPH::MutableCompoundShape* compoundShape;
        if (m_colliders.find(tag.getUuid()) == m_colliders.end()) {
            m_colliders[tag.getUuid()] = new JPH::MutableCompoundShape();
        }
        compoundShape = m_colliders[tag.getUuid()];
        JPH::Ref<JPH::BoxShape> shape = new JPH::BoxShape(size);
        JPH::Ref<JPH::ScaledShape> scaledShape = new JPH::ScaledShape(shape, JPH::RVec3(scale.x, scale.y, scale.z));
        compoundShape->AddShape(JPH::RVec3(center.x, center.y, center.z), JPH::Quat::sIdentity(), scaledShape);
        if (m_running && m_bodies.find(tag.getUuid()) != m_bodies.end()) {
            JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
            JPH::BodyID body = m_bodies[tag.getUuid()];
            bodyInterface.SetShape(body, compoundShape, true, JPH::EActivation::DontActivate);
            //bodyInterface.AddBody(body, JPH::EActivation::Activate);
        }
    }

    const void PhysicsEngine::createSphereCollider(const Tag& tag, const Transform& transform, glm::vec3 center, float radius) {
        JPH::RegisterDefaultAllocator();
        JPH::MutableCompoundShape* compoundShape;
        if (m_colliders.find(tag.getUuid()) == m_colliders.end()) {
            m_colliders[tag.getUuid()] = new JPH::MutableCompoundShape();
        }
        compoundShape = m_colliders[tag.getUuid()];
        JPH::Ref<JPH::SphereShape> shape = new JPH::SphereShape(radius);
        glm::quat rotation = glm::quat(glm::radians(transform.rotation));
        JPH::Ref<JPH::ScaledShape> scaledShape = new JPH::ScaledShape(shape, JPH::RVec3(1.0f, 1.0f, 1.0f));
        compoundShape->AddShape(JPH::RVec3(center.x, center.y, center.z), JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w), scaledShape);
    }

    const void PhysicsEngine::moveOrRotateBody(const Tag& tag, const Transform& transform) {
        if (m_running) {
            return;
        }
        if (m_bodies.find(tag.getUuid()) != m_bodies.end()) {
            JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
            JPH::BodyID body = m_bodies[tag.getUuid()];
            JPH::RVec3 position = JPH::RVec3(transform.position.x, transform.position.y, transform.position.z);
            glm::quat rotation = glm::quat(glm::radians(transform.rotation));
            JPH::Quat quat = JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w);
            bodyInterface.SetPositionAndRotation(body, position, quat, JPH::EActivation::DontActivate);
        }
    }

    const void PhysicsEngine::resizeCollider(const Tag& tag, Transform& transform, glm::vec3 center, glm::vec3 scale) {
        JPH::Ref<JPH::MutableCompoundShape> compoundShape = m_colliders[tag.getUuid()];
        compoundShape->RemoveShape(0);
        createBoxCollider(tag, transform, center, scale);
    }

    const void PhysicsEngine::recenterCollider(const Tag& tag, glm::vec3 center) {
        JPH::Ref<JPH::MutableCompoundShape> shape = m_colliders[tag.getUuid()];
        shape->ModifyShape(0, JPH::RVec3(center.x, center.y, center.z), shape->GetSubShape(0).GetRotation());
    }

    const void PhysicsEngine::rescaleCollider(const Tag& tag, glm::vec3 center, glm::vec3 size) {
        JPH::Ref<JPH::MutableCompoundShape> shape = m_colliders[tag.getUuid()];
        JPH::RVec3 scale = JPH::RVec3(size.x, size.y, size.z);
        auto* scaledShape = static_cast<const JPH::ScaledShape*>(shape->GetSubShape(0).mShape.GetPtr());
        auto* originalShape = (JPH::BoxShape*)scaledShape->GetInnerShape();
        scaledShape = new JPH::ScaledShape(originalShape, scale);
        shape->RemoveShape(0);
        shape->AddShape(JPH::RVec3(center.x, center.y, center.z), JPH::Quat::sIdentity(), scaledShape);
    }

    void PhysicsEngine::removeBody(const Tag& tag) {
        if (m_bodies.find(tag.getUuid()) != m_bodies.end()) {
            if (m_running) {
                JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
                bodyInterface.RemoveBody(m_bodies[tag.getUuid()]);
            }
            m_bodies.erase(tag.getUuid());
        }
    }

    void PhysicsEngine::removeCollider(const Tag& tag) {
        if (m_colliders.find(tag.getUuid()) != m_colliders.end()) {
            if (m_running && m_bodies.find(tag.getUuid()) != m_bodies.end()) {
                JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
                JPH::BodyID body = m_bodies[tag.getUuid()];
                bodyInterface.SetShape(body, new JPH::EmptyShape(), true, JPH::EActivation::DontActivate);
                bodyInterface.AddBody(body, JPH::EActivation::Activate);
            }
            m_colliders.erase(tag.getUuid());
        }
    }

    void PhysicsEngine::updateBodies() {
        Scene& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
        scene.runSystem<Transform, StaticBody>([this](Transform& transform, StaticBody& body) {
            updateBodies<StaticBody>(transform, body);
        });
        scene.runSystem<Transform, RigidBody>([this](Transform& transform, RigidBody& body) {
            updateBodies<RigidBody>(transform, body);
        });
        /*scene.runSystem<Transform, SphereCollider>([this](Transform& transform, SphereCollider& collider) {
            updateBodies<SphereCollider>(transform, collider);
        });*/
    }

    void PhysicsEngine::updateEntities() {
        Scene& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
        scene.runSystem<Transform, RigidBody>([this](Transform& transform, RigidBody& body) {
            updateEntities<RigidBody>(transform, body);
        });
        /*scene.runSystem<Transform, SphereCollider>([this](Transform& transform, SphereCollider& collider) {
            updateEntities<SphereCollider>(transform, collider);
        });*/
    }

    const JPH::BodyID& PhysicsEngine::createBody(JPH::EMotionType eMotionType, const Tag& tag, const Transform& transform) {
        JPH::RegisterDefaultAllocator();
        JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();

        glm::quat quat = glm::quat(glm::radians(transform.rotation));

        JPH::Shape* shape = new JPH::EmptyShape();
        JPH::RVec3 position = JPH::RVec3(transform.position.x, transform.position.y, transform.position.z);
        JPH::Quat rotation = JPH::Quat(quat.x, quat.y, quat.z, quat.w);
        JPH::BodyCreationSettings settings(shape, position, rotation, eMotionType, Layers::MOVING);

        JPH::Body* body = bodyInterface.CreateBody(settings);
        if (m_bodies.find(tag.getUuid()) == m_bodies.end()) {
            m_bodies[tag.getUuid()] = {};
        }
        m_bodies[tag.getUuid()] = body->GetID();
        return body->GetID();
    }
}
