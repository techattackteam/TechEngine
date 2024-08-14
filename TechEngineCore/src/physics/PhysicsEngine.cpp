#include "PhysicsEngine.hpp"
#include <Jolt/jolt.h>
#include "components/physics/BoxColliderComponent.hpp"
#include "components/physics/SphereCollider.hpp"
#include "MyBodyActivationListener.hpp"
#include "MyContactListener.hpp"

#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>

#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>

#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>

#include <Jolt/Renderer/DebugRendererRecorder.h>

#include "BPLayerInterfaceImpl.hpp"
#include "ObjectLayerPairFilterImpl.hpp"
#include "ObjectVsBroadPhaseLayerFilterImpl.hpp"
#include "core/Timer.hpp"
#include "scene/SceneManager.hpp"


namespace TechEngine {
    PhysicsEngine::PhysicsEngine(SystemsRegistry& systemsRegistry) : systemsRegistry(systemsRegistry) {
        /*callback = new PhysicsCallback(eventDispatcher, scene, *this);

        eventDispatcher.subscribe(GameObjectDestroyEvent::eventType, [this](Event* event) {
            std::string tag = ((GameObjectDestroyEvent*)event)->getGameObjectTag();
            removeActor(tag);
        });
        eventDispatcher.subscribe(AddColliderEvent::eventType, [this](Event* event) {
            addCollider(((AddColliderEvent*)event)->getCollider());
        });
        eventDispatcher.subscribe(RemoveColliderEvent::eventType, [this](Event* event) {
            removeCollider(((RemoveColliderEvent*)event)->getTag(), ((RemoveColliderEvent*)event)->getCollider());
        });

        eventDispatcher.subscribe(AddRigidBodyEvent::eventType, [this](Event* event) {
            addRigidBody(((AddRigidBodyEvent*)event)->getRigidBody());
        });

        eventDispatcher.subscribe(RemoveRigidBodyEvent::eventType, [this](Event* event) {
            removeRigidBody(((RemoveRigidBodyEvent*)event)->getRigidBody()->getGameObject()->getTag());
        });*/
    }


    PhysicsEngine::~PhysicsEngine() {
        /*pxScene->release();
        dispatcher->release();
        physics->release();
        pvd->release();
        foundation->release();
        delete callback;*/
    }

    void PhysicsEngine::runSimulation() {
    }

    static void traceImpl(const char* inFMT, ...) {
        va_list list;
        va_start(list, inFMT);
        char buffer[1024];
        vsprintf_s(buffer, inFMT, list);
        va_end(list);
        TE_LOGGER_CRITICAL(buffer);
    }

    // Callback for asserts, connect this to your own assert handler if you have one
    static bool AssertFailedImpl(const char* inExpression, const char* inMessage, const char* inFile, JPH::uint inLine) {
        // Print to the TTY
        TE_LOGGER_CRITICAL("{0}:{1}: ({2}) {3}", inFile, inLine, inExpression, inMessage);
        // Breakpoint
        return true;
    };

    void PhysicsEngine::init() {
        JPH::RegisterDefaultAllocator();
        temp_allocator = new JPH::TempAllocatorImpl(10 * 1024 * 1024);
        job_system = new JPH::JobSystemThreadPool(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

        // Install trace and assert callbacks
        JPH::Trace = traceImpl;
        JPH_IF_ENABLE_ASSERTS(JPH::AssertFailed = AssertFailedImpl;) // Note: This is a macro that will expand to nothing if asserts are disabled
        // Create a factory, this class is responsible for creating instances of classes based on their name or hash and is mainly used for deserialization of saved data.
        // It is not directly used in this example but still required.
        JPH::Factory::sInstance = new JPH::Factory();

        // Create mapping table from object layer to broadphase layer
        // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
        JPH::RegisterTypes();

        broad_phase_layer_interface = new BPLayerInterfaceImpl();

        // Create class that filters object vs broadphase layers
        // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
        object_vs_broadphase_layer_filter = new ObjectVsBroadPhaseLayerFilterImpl();

        // Create class that filters object vs object layers
        // Note: As this is an interface, PhysicsSystem will take a reference to this so this instance needs to stay alive!
        object_vs_object_layer_filter = new ObjectLayerPairFilterImpl();

        physics_system = new JPH::PhysicsSystem();

        physics_system->Init(cMaxBodies, cNumBodyMutexes, cMaxBodyPairs, cMaxContactConstraints, *broad_phase_layer_interface, *object_vs_broadphase_layer_filter, *object_vs_object_layer_filter);
    }


    void PhysicsEngine::start() {
        running = true;
    }

    void PhysicsEngine::stop() {
        running = false;
    }

    void PhysicsEngine::onFixedUpdate() {
        if (!running) {
            return;
        }
        Timer& timer = systemsRegistry.getSystem<Timer>();
        if (bodyIDs.empty()) {
            return;
        }

        updateBodies();
        float timeStep = timer.getDeltaTime();
        if (timeStep >= 1.0f) { // If the time step is too large, too many collision steps are required and will run out of memory
            return;
        }
        int numSteps = static_cast<int>(std::ceil(timeStep / timer.getTPS()));

        physics_system->Update(timer.getTPS(), numSteps, temp_allocator, job_system);
        TE_LOGGER_INFO("timeSetup: {0} NumSteps: {1}", timeStep, numSteps);
        updateGameObjects();
    }

    void PhysicsEngine::clear() {
        /*for (auto bodyID: bodyIDs) {
            physics_system->GetBodyInterface().RemoveBody(bodyID);
        }
        bodyIDs.clear();*/
    }

    void PhysicsEngine::addCollider(Collider* collider) {
        JPH::RegisterDefaultAllocator(); // For some reason this is needed here otherwise the program crashes
        TransformComponent* transform = collider->getGameObject()->getComponent<TransformComponent>();
        glm::vec3 position = transform->getWorldPosition();
        glm::quat rotation = glm::quat(glm::radians(transform->getOrientation()));
        JPH::Ref<JPH::Shape> shape = createGeometry(*transform, collider);
        JPH::BodyCreationSettings body_creation_settings(shape,
                                                         JPH::RVec3(position.x, position.y, position.z),
                                                         JPH::Quat(rotation.x, rotation.y, rotation.z, rotation.w),
                                                         JPH::EMotionType::Dynamic,
                                                         Layers::MOVING);
        JPH::BodyID bodyID = physics_system->GetBodyInterface().CreateAndAddBody(body_creation_settings, JPH::EActivation::Activate);
        JPH::BodyInterface& body_interface = physics_system->GetBodyInterface();
        CustomData* customData = new CustomData{collider->getGameObject()->getTag()};
        body_interface.SetUserData(bodyID, reinterpret_cast<uint64_t>(customData));
        bodyIDs.push_back(bodyID);
    }

    void PhysicsEngine::addRigidBody(RigidBody* rigidBody) {
    }

    void PhysicsEngine::removeCollider(const std::string& gameObjectTag, Collider* collider) {
    }

    void PhysicsEngine::removeRigidBody(const std::string& tag) {
    }

    void PhysicsEngine::drawDebug(JPH::DebugRenderer& debugRenderer) {
        physics_system->DrawBodies(JPH::BodyManager::DrawSettings(), &debugRenderer);
    }


    JPH::Shape* PhysicsEngine::createGeometry(TransformComponent& transform, Collider* collider) {
        if (BoxColliderComponent* boxCollider = dynamic_cast<BoxColliderComponent*>(collider)) {
            glm::vec3 size = transform.getScale() * boxCollider->getSize();
            JPH::Vec3Arg vec3 = JPH::Vec3(size.x, size.y, size.z);
            return new JPH::BoxShape(vec3);
        } else if (SphereCollider* sphereCollider = dynamic_cast<SphereCollider*>(collider)) {
            return new JPH::SphereShape(0.5f);
        } else if (CylinderCollider* cylinderCollider = dynamic_cast<CylinderCollider*>(collider)) {
            return new JPH::CylinderShape(cylinderCollider->getHeight() / 2, cylinderCollider->getRadius());
        } else {
            TE_LOGGER_CRITICAL("Collider type not supported!");
            return nullptr;
        }
    }

    void PhysicsEngine::updateBodies() {
        for (auto bodyID: bodyIDs) {
            CustomData* customData = reinterpret_cast<CustomData*>(physics_system->GetBodyInterface().GetUserData(bodyID));
            TransformComponent* transform = systemsRegistry.getSystem<SceneManager>().getScene().getGameObjectByTag(customData->tag)->getComponent<TransformComponent>();
            JPH::RVec3 position = physics_system->GetBodyInterface().GetCenterOfMassPosition(bodyID);
            if (transform->position.x != position[0] || transform->position.y != position[1] || transform->position.z != position[2]) {
                glm::quat rotation = glm::quat(transform->getOrientation());
                physics_system->GetBodyInterface().SetPositionRotationAndVelocity(bodyID,
                                                                                  JPH::RVec3(transform->position.x, transform->position.y, transform->position.z),
                                                                                  JPH::Quat(rotation.w, rotation.x, rotation.y, rotation.z),
                                                                                  JPH::Vec3(0, 0, 0),
                                                                                  JPH::Vec3(0, 0, 0));
            }
        }
    }

    void PhysicsEngine::updateGameObjects() {
        for (auto bodyID: bodyIDs) {
            CustomData* customData = reinterpret_cast<CustomData*>(physics_system->GetBodyInterface().GetUserData(bodyID));
            JPH::Vec3 position = physics_system->GetBodyInterface().GetCenterOfMassPosition(bodyID);
            JPH::Quat rotation = physics_system->GetBodyInterface().GetRotation(bodyID);
            SceneManager& sceneManager = systemsRegistry.getSystem<SceneManager>();
            TransformComponent* transform = sceneManager.getScene().getGameObjectByTag(customData->tag)->getComponent<TransformComponent>();
            transform->translateToWorld(glm::vec3(position[0], position[1], position[2]));
            transform->setRotation(glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ()));
        }
    }
}
