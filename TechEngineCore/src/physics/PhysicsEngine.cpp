#include "PhysicsEngine.hpp"
#include "core/Logger.hpp"
#include "core/Timer.hpp"
#include "components/TransformComponent.hpp"
#include "scene/Scene.hpp"
#include "components/physics/SphereCollider.hpp"
#include "event/EventDispatcher.hpp"
#include <typeinfo>

namespace TechEngine {
    PhysicsEngine::PhysicsEngine() {
        if (instance != nullptr) {
            delete this;
        } else {
            instance = this;
        }
        EventDispatcher::getInstance().subscribe(RequestDeleteGameObject::eventType, [this](Event *event) {
            removeActor(((RequestDeleteGameObject *) event)->getGameObject());
        });
    }

    PhysicsEngine::~PhysicsEngine() {
        scene->release();
        dispatcher->release();
        physics->release();
        pvd->release();
        foundation->release();
    }

    void PhysicsEngine::init() {
        foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
        if (!foundation)
            TE_LOGGER_CRITICAL("PxCreateFoundation failed!");
        pvd = PxCreatePvd(*foundation);
        physx::PxPvdTransport *transport = physx::PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
        pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
        physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, physx::PxTolerancesScale(), true, pvd);
        if (!physics)
            TE_LOGGER_CRITICAL("PxCreatePhysics failed!");

        defaultMaterial = physics->createMaterial(0.5f, 0.5f, 0.0f);
        createScene();
        TE_LOGGER_INFO("Physics engine initialized!");
    }

    void PhysicsEngine::createScene() {
        physx::PxSceneDesc sceneDesc(physics->getTolerancesScale());
        sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
        dispatcher = physx::PxDefaultCpuDispatcherCreate(2);
        sceneDesc.cpuDispatcher = dispatcher;
        sceneDesc.filterShader = physx::PxDefaultSimulationFilterShader;
        scene = physics->createScene(sceneDesc);
        if (!scene)
            TE_LOGGER_CRITICAL("createScene failed!");
    }

    void PhysicsEngine::start() {
        if (!scene)
            TE_LOGGER_CRITICAL("createScene failed!");
        for (auto &actor: actors) {
            GameObject *gameObject = Scene::getInstance().getGameObjectByTag(actor.first);
            TransformComponent *transformComponent = gameObject->getComponent<TransformComponent>();
            glm::vec3 position = transformComponent->getPosition();
            glm::quat rotation = glm::quat(glm::radians(transformComponent->getOrientation()));
            actor.second->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
            if (actor.second->getNbShapes()) {

            }
            scene->addActor(*actor.second);
        }
        running = true;
    }

    void PhysicsEngine::stop() {
        scene->release();
        createScene();
        running = false;
    }

    void PhysicsEngine::onUpdate() {
        if (running) {
            updateActorPositions();
            scene->simulate(Timer::getInstance().getTPS());
            scene->fetchResults(true);
            updateGameObjectPositions();
        }
    }

    void PhysicsEngine::updateActorPositions() {
        for (auto &actor: actors) {
            TransformComponent *transformComponent = static_cast<TransformComponent *>(actor.second->userData);
            glm::vec3 position = transformComponent->getWorldPosition();
            glm::quat rotation = glm::quat(glm::radians(transformComponent->getOrientation()));
            actor.second->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
        }
    }

    void PhysicsEngine::updateGameObjectPositions() {
        for (auto &actor: actors) {
            physx::PxTransform transform = actor.second->getGlobalPose();
            TransformComponent *transformComponent = static_cast<TransformComponent *>(actor.second->userData);
            transformComponent->translateToWorld(glm::vec3(transform.p.x, transform.p.y, transform.p.z));
            transformComponent->setRotation(glm::quat(transform.q.w, transform.q.x, transform.q.y, transform.q.z));
        }
    }

    void PhysicsEngine::addCollider(Collider *collider) {
        removeActor(collider);
        TransformComponent t = collider->getTransform();
        glm::vec3 position = t.getWorldPosition();
        glm::quat rotation = glm::quat(glm::radians(t.getOrientation()));
        physx::PxTransform transform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w));
        if (BoxColliderComponent *boxCollider = dynamic_cast<BoxColliderComponent *>(collider)) {
            glm::vec3 size = t.getScale() / glm::vec3(2, 2, 2) * boxCollider->getSize();
            physx::PxBoxGeometry geometry(size.x, size.y, size.z);
            addActor(boxCollider, transform, geometry);
        } else if (SphereCollider *sphereCollider = dynamic_cast<SphereCollider *>(collider)) {
            physx::PxSphereGeometry geometry(sphereCollider->getRadius());
            addActor(sphereCollider, transform, geometry);
        } else {
            TE_LOGGER_CRITICAL("Collider type not supported!");
        }
    }

    void PhysicsEngine::addActor(Collider *collider, physx::PxTransform transform, const physx::PxGeometry &actor) {
        if (collider->isDynamic()) {
            addDynamicActor(transform, actor, collider);
        } else {
            addStaticActor(transform, actor, collider);
        }
    }

    void PhysicsEngine::addDynamicActor(physx::PxTransform transform, const physx::PxGeometry &geometry, Collider *collider) {
        physx::PxRigidDynamic *dynamicActor = PxCreateDynamic(*physics, transform, geometry, *defaultMaterial, 10.0f);
        dynamicActor->userData = collider->getGameObject()->getComponent<TransformComponent>();
        scene->addActor(*dynamicActor);
        actors[collider->getGameObject()->getTag()] = dynamicActor;
    }

    void PhysicsEngine::addStaticActor(physx::PxTransform transform, const physx::PxGeometry &geometry, Collider *collider) {
        physx::PxRigidStatic *staticActor = PxCreateStatic(*physics, transform, geometry, *defaultMaterial);
        staticActor->userData = collider->getGameObject()->getComponent<TransformComponent>();
        scene->addActor(*staticActor);
        actors[collider->getGameObject()->getTag()] = staticActor;
    }

    void PhysicsEngine::removeActor(Collider *collider) {
        if (actors.find(collider->getGameObject()->getTag()) != actors.end()) {
            actors[collider->getGameObject()->getTag()]->release();
            actors.erase(collider->getGameObject()->getTag());
        }
    }

    void PhysicsEngine::removeActor(GameObject *gameObject) {
        if (actors.find(gameObject->getTag()) != actors.end()) {
            actors[gameObject->getTag()]->release();
            actors.erase(gameObject->getTag());
        }
    }

    PhysicsEngine *PhysicsEngine::getInstance() {
        if (instance == nullptr) {
            return new PhysicsEngine();
        } else {
            return instance;
        };
    }


}
