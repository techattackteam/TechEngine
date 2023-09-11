#include "PhysicsEngine.hpp"
#include "core/Logger.hpp"
#include "core/Timer.hpp"
#include "components/TransformComponent.hpp"
#include "scene/Scene.hpp"

namespace TechEngine {
    PhysicsEngine::PhysicsEngine() {
        if (instance != nullptr) {
            delete this;
        } else {
            instance = this;
        }
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
            glm::vec3 position = transformComponent->getPosition();
            glm::quat rotation = glm::quat(glm::radians(transformComponent->getOrientation()));
            actor.second->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
        }
    }

    void PhysicsEngine::updateGameObjectPositions() {
        for (auto &actor: actors) {
            physx::PxTransform transform = actor.second->getGlobalPose();
            TransformComponent *transformComponent = static_cast<TransformComponent *>(actor.second->userData);
            transformComponent->translateTo(glm::vec3(transform.p.x, transform.p.y, transform.p.z));
            transformComponent->setRotation(glm::quat(transform.q.w, transform.q.x, transform.q.y, transform.q.z));
        }
    }


    void PhysicsEngine::addBoxCollider(BoxColliderComponent *boxColliderComponent) {
        if (actors.find(boxColliderComponent->getGameObject()->getTag()) != actors.end()) {
            actors[boxColliderComponent->getGameObject()->getTag()]->release();
            actors.erase(boxColliderComponent->getGameObject()->getTag());
        }
        TransformComponent t = boxColliderComponent->getTransform();
        glm::vec3 position = t.getPosition() + boxColliderComponent->getOffset();
        glm::quat rotation = glm::quat(glm::radians(t.getOrientation()));
        physx::PxBoxGeometry boxGeometry(t.getScale().x / 2, t.getScale().y / 2, t.getScale().z / 2);
        physx::PxTransform transform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w));
        if (boxColliderComponent->isDynamic()) {
            physx::PxRigidDynamic *dynamicActor = PxCreateDynamic(*physics, transform, boxGeometry, *defaultMaterial, 10.0f);
            dynamicActor->userData = boxColliderComponent->getGameObject()->getComponent<TransformComponent>();
            scene->addActor(*dynamicActor);
            actors[boxColliderComponent->getGameObject()->getTag()] = dynamicActor;
        } else {
            physx::PxRigidStatic *staticActor = PxCreateStatic(*physics, transform, boxGeometry, *defaultMaterial);
            staticActor->userData = boxColliderComponent->getGameObject()->getComponent<TransformComponent>();
            scene->addActor(*staticActor);
            actors[boxColliderComponent->getGameObject()->getTag()] = staticActor;
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
