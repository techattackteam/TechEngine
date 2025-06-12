#include "PhysicsEngine.hpp"
#include <Jolt/Jolt.h>


#include "core/Logger.hpp"
#include "core/Timer.hpp"
#include "systems/SystemsRegistry.hpp"
//#include "physics/DebugRenderer.hpp"

#include <iostream>
#include <cstdarg>
#include <thread>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Collision/Shape/EmptyShape.h>
#include <Jolt/Physics/Collision/Shape/MutableCompoundShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>

#include "scene/ScenesManager.hpp"

// Disable common warnings triggered by Jolt, you can use JPH_SUPPRESS_WARNING_PUSH / JPH_SUPPRESS_WARNING_POP to store and restore the warning state
JPH_SUPPRESS_WARNINGS


namespace TechEngine {
    PhysicsEngine::PhysicsEngine(SystemsRegistry& systemsRegistry) : System(),
                                                                     m_systemsRegistry(systemsRegistry),
                                                                     contact_listener(systemsRegistry) {
    }

    void PhysicsEngine::init() {
        JPH::RegisterDefaultAllocator();
        JPH::Trace = TraceImpl;
        JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;)

        JPH::Factory::sInstance = new JPH::Factory();

        JPH::RegisterTypes();
        //debugRenderer = new DebugRenderer();

        temp_allocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);

        job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers,
                                                  JPH::thread::hardware_concurrency() - 1);

        const JPH::uint cMaxBodies = 1024;
        const JPH::uint cNumBodyMutexes = 0;
        const JPH::uint cMaxBodyPairs = 1024;
        const JPH::uint cMaxContactConstraints = 1024;


        m_physicsSystem = new JPH::PhysicsSystem();
        m_physicsSystem->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints,
                              broad_phase_layer_interface, object_vs_broadphase_layer_filter,
                              object_vs_object_layer_filter);
        // A body activation listener gets notified when bodies activate and go to sleep
        // Note that this is called from a job so whatever you do here needs to be thread safe.
        // Registering one is entirely optional.
        //m_physicsSystem->SetBodyActivationListener(&body_activation_listener);

        // A contact listener gets notified when bodies (are about to) collide, and when they separate again.
        // Note that this is called from a job so whatever you do here needs to be thread safe.
        // Registering one is entirely optional.
        m_physicsSystem->SetContactListener(&contact_listener);
        contact_listener.init(&m_bodies,
                              &m_triggers,
                              m_physicsSystem);
    }

    void PhysicsEngine::shutdown() {
        stop();
        JPH::UnregisterTypes();
        delete temp_allocator;
        delete job_system;
        //delete debugRenderer;
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
        delete m_physicsSystem;
    }

    bool PhysicsEngine::start() {
        JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
        m_running = true;
        for (auto& [uuid, body]: m_bodies) {
            if (!m_colliders.contains(uuid) && !m_triggers.contains(uuid)) {
                TE_LOGGER_ERROR("Body with uuid: {0} has no collider or trigger", uuid.c_str());
                return false;
            }
            if (m_triggers.contains(uuid)) {
                bodyInterface.AddBody(body, JPH::EActivation::DontActivate);
                bodyInterface.AddBody(m_triggers[uuid], JPH::EActivation::Activate);
            }
            if (m_colliders.contains(uuid)) {
                bodyInterface.SetShape(body, m_colliders[uuid], true, JPH::EActivation::DontActivate);
                bodyInterface.AddBody(body, JPH::EActivation::Activate);
            }
        }
        return true;
    }

    void PhysicsEngine::stop() {
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

    void PhysicsEngine::renderBodies() const {
#ifdef PHYSICS_DEBUG_RENDERER
       //if (debugRenderer != nullptr) {
       //    m_physicsSystem->DrawBodies(JPH::BodyManager::DrawSettings(), debugRenderer);
       //}
#endif
    }

    const JPH::BodyID& PhysicsEngine::createStaticBody(const Tag& tag, const Transform& transform) {
        return createBody(JPH::EMotionType::Static, tag, transform, false);
    }

    const JPH::BodyID& PhysicsEngine::createKinematicBody(const Tag& tag, const Transform& transform) {
        return createBody(JPH::EMotionType::Kinematic, tag, transform, false);
    }

    const JPH::BodyID& PhysicsEngine::createRigidBody(const Tag& tag, const Transform& transform) {
        return createBody(JPH::EMotionType::Dynamic, tag, transform, false);
    }

    void PhysicsEngine::createCollider(const Shape shape, const Tag& tag, const Transform& transform, glm::vec3 center,
                                       glm::vec3 size) {
        JPH::RegisterDefaultAllocator();
        JPH::MutableCompoundShape* compoundShape = ShapeFactory::createShape(shape, transform, center, size);
        m_colliders[tag.getUuid()] = compoundShape;
        if (m_running && m_bodies.contains(tag.getUuid())) {
            const JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
            const JPH::BodyID body = m_bodies[tag.getUuid()];
            bodyInterface.SetShape(body, compoundShape, true, JPH::EActivation::DontActivate);
        }
    }

    const JPH::BodyID& PhysicsEngine::createTrigger(const Shape shape, const Tag& tag, const Transform& transform,
                                                    const glm::vec3 center, const glm::vec3 size) {
        const JPH::BodyID& body = createBody(JPH::EMotionType::Static, tag, transform, true);
        m_triggers[tag.getUuid()] = body;
        JPH::MutableCompoundShape* compoundShape = ShapeFactory::createShape(shape, transform, center, size);
        JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
        bodyInterface.SetShape(body, compoundShape, true, JPH::EActivation::DontActivate);
        return body;
    }

    void PhysicsEngine::resizeCollider(const Shape shape, const Tag& tag, const Transform& transform,
                                       const glm::vec3 center, const glm::vec3 size) { // Size -> (radius, height)
        const JPH::Ref<JPH::MutableCompoundShape> compoundShape = m_colliders[tag.getUuid()];
        compoundShape->RemoveShape(0);
        createCollider(shape, tag, transform, center, size);
    }

    void PhysicsEngine::resizeTrigger(const Shape shape, const Tag& tag, const Transform& transform,
                                      const glm::vec3 center, const glm::vec3 size) {
        JPH::BodyID& bodyID = m_triggers[tag.getUuid()];
        JPH::MutableCompoundShape* compoundShape = ShapeFactory::createShape(shape, transform, center, size);
        m_physicsSystem->GetBodyInterface().SetShape(bodyID, compoundShape, true, JPH::EActivation::DontActivate);
    }

    void PhysicsEngine::recenterCollider(const Tag& tag, glm::vec3 center) {
        JPH::Ref<JPH::MutableCompoundShape> shape = m_colliders[tag.getUuid()];
        shape->ModifyShape(0, JPH::RVec3(center.x, center.y, center.z), shape->GetSubShape(0).GetRotation());
    }

    void PhysicsEngine::recenterTrigger(const Tag& tag, glm::vec3 center) {
        JPH::BodyID& bodyID = m_triggers[tag.getUuid()];
        const JPH::BodyLockInterfaceLocking& lockInterface = m_physicsSystem->GetBodyLockInterface();
        // Or GetBodyLockInterfaceNoLock
        // Scoped lock
        JPH::Ref<JPH::MutableCompoundShape> shape; //
        {
            JPH::BodyLockRead lock(lockInterface, bodyID);
            if (lock.Succeeded()) {
                const JPH::Body& body = lock.GetBody();
                const JPH::MutableCompoundShape* constShape = dynamic_cast<const JPH::MutableCompoundShape*>(body.
                    GetShape());
                shape = constShape->Clone();
            } else {
                return;
            }
        }
        shape->ModifyShape(0, JPH::RVec3(center.x, center.y, center.z), shape->GetSubShape(0).GetRotation());
        m_physicsSystem->GetBodyInterface().SetShape(bodyID, shape, true, JPH::EActivation::DontActivate);
    }

    void PhysicsEngine::rescaleCollider(const Tag& tag, const Transform& transform, const glm::vec3 center,
                                        const bool uniform) {
        const JPH::Ref<JPH::MutableCompoundShape> shape = m_colliders[tag.getUuid()];
        JPH::RVec3 scale;
        if (uniform) {
            const float maxTransformScale = std::max(std::max(transform.scale.x, transform.scale.y), transform.scale.z);
            scale = JPH::RVec3(maxTransformScale, maxTransformScale, maxTransformScale);
        } else {
            scale = JPH::RVec3(transform.scale.x,
                               transform.scale.y,
                               transform.scale.z);
        }
        auto* scaledShape = static_cast<const JPH::ScaledShape*>(shape->GetSubShape(0).mShape.GetPtr());
        auto* originalShape = scaledShape->GetInnerShape();
        scaledShape = new JPH::ScaledShape(originalShape, scale);
        shape->RemoveShape(0);
        shape->AddShape(JPH::RVec3(center.x, center.y, center.z), JPH::Quat::sIdentity(), scaledShape);
    }

    void PhysicsEngine::rescaleTrigger(const Tag& tag, const Transform& transform, glm::vec3 center,
                                       const bool uniform) {
        JPH::BodyID& bodyID = m_triggers[tag.getUuid()];
        const JPH::BodyLockInterfaceLocking& lockInterface = m_physicsSystem->GetBodyLockInterface();
        // Or GetBodyLockInterfaceNoLock
        // Scoped lock
        JPH::Ref<JPH::MutableCompoundShape> shape; //
        {
            JPH::BodyLockRead lock(lockInterface, bodyID);
            if (lock.Succeeded()) // body_id may no longer be valid
            {
                const JPH::Body& body = lock.GetBody();
                const auto* constShape = dynamic_cast<const JPH::MutableCompoundShape*>(body.GetShape());
                shape = constShape->Clone();
            } else {
                return;
            }
        }
        JPH::RVec3 scale;
        if (uniform) {
            const float maxTransformScale = std::max(std::max(transform.scale.x, transform.scale.y), transform.scale.z);
            scale = JPH::RVec3(maxTransformScale, maxTransformScale, maxTransformScale);
        } else {
            scale = JPH::RVec3(transform.scale.x,
                               transform.scale.y,
                               transform.scale.z);
        }
        auto* scaledShape = dynamic_cast<const JPH::ScaledShape*>(shape->GetSubShape(0).mShape.GetPtr());
        auto* originalShape = scaledShape->GetInnerShape();
        scaledShape = new JPH::ScaledShape(originalShape, scale);
        shape->RemoveShape(0);
        shape->AddShape(JPH::RVec3(center.x, center.y, center.z), JPH::Quat::sIdentity(), scaledShape);
        m_physicsSystem->GetBodyInterface().SetShape(bodyID, shape, true, JPH::EActivation::DontActivate);
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
        if (m_colliders.contains(tag.getUuid())) {
            if (m_running && m_bodies.contains(tag.getUuid())) {
                JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
                JPH::BodyID body = m_bodies[tag.getUuid()];
                bodyInterface.SetShape(body, new JPH::EmptyShape(), true, JPH::EActivation::DontActivate);
                bodyInterface.AddBody(body, JPH::EActivation::Activate);
            }
            m_colliders.erase(tag.getUuid());
        }
    }

    void PhysicsEngine::removeTrigger(const Tag& tag) {
        if (m_colliders.contains(tag.getUuid())) {
            if (m_running && m_triggers.contains(tag.getUuid())) {
                JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
                bodyInterface.RemoveBody(m_triggers[tag.getUuid()]);
            }
            m_triggers.erase(tag.getUuid());
        }
    }

    void PhysicsEngine::setVelocity(const Tag& tag, const glm::vec3& velocity) {
        JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
        JPH::BodyID body = m_bodies[tag.getUuid()];
        bodyInterface.SetLinearVelocity(body, JPH::RVec3(velocity.x, velocity.y, velocity.z));
    }

    void PhysicsEngine::updateBodies() {
        Scene& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
        scene.runSystem<Transform, StaticBody>([this](Transform& transform, StaticBody& body) {
            updateBodies<StaticBody>(transform, body);
        });
        scene.runSystem<Transform, RigidBody>([this](Transform& transform, RigidBody& body) {
            updateBodies<RigidBody>(transform, body);
        });
        scene.runSystem<Transform, BoxTrigger>([this](Transform& transform, BoxTrigger& trigger) {
            updateBodies<BoxTrigger>(transform, trigger);
        });
    }

    void PhysicsEngine::updateEntities() {
        Scene& scene = m_systemsRegistry.getSystem<ScenesManager>().getActiveScene();
        scene.runSystem<Transform, RigidBody>([this](Transform& transform, RigidBody& body) {
            updateEntities<RigidBody>(transform, body);
        });
        scene.runSystem<Transform, BoxTrigger>([this](Transform& transform, BoxTrigger& body) {
            updateEntities<BoxTrigger>(transform, body);
        });
        /*scene.runSystem<Transform, SphereCollider>([this](Transform& transform, SphereCollider& collider) {
            updateEntities<SphereCollider>(transform, collider);
        });*/
    }

    void PhysicsEngine::moveOrRotateBody(const Tag& tag, const Transform& transform) {
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

    const JPH::BodyID& PhysicsEngine::createBody(JPH::EMotionType eMotionType, const Tag& tag,
                                                 const Transform& transform, bool isTrigger) {
        JPH::RegisterDefaultAllocator();
        JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();

        glm::quat quat = glm::quat(glm::radians(transform.rotation));

        JPH::Shape* shape = new JPH::EmptyShape();
        JPH::RVec3 position = JPH::RVec3(transform.position.x, transform.position.y, transform.position.z);
        JPH::Quat rotation = JPH::Quat(quat.x, quat.y, quat.z, quat.w);
        JPH::BodyCreationSettings settings(shape, position, rotation, eMotionType, Layers::MOVING);

        JPH::Body* body = bodyInterface.CreateBody(settings);
        if (isTrigger) {
            body->SetIsSensor(isTrigger);
            if (m_triggers.find(tag.getUuid()) == m_triggers.end()) {
                m_triggers[tag.getUuid()] = {};
            }
            m_triggers[tag.getUuid()] = body->GetID();
        } else {
            if (m_bodies.find(tag.getUuid()) == m_bodies.end()) {
                m_bodies[tag.getUuid()] = {};
            }
            m_bodies[tag.getUuid()] = body->GetID();
        }
        if (m_running) {
            bodyInterface.AddBody(body->GetID(), JPH::EActivation::Activate);
            if (m_bodies.contains(tag.getUuid())) {
                JPH::Shape* shape = m_colliders[tag.getUuid()];
                bodyInterface.SetShape(body->GetID(), shape, true, JPH::EActivation::DontActivate);
            }
        }
        return body->GetID();
    }
}
