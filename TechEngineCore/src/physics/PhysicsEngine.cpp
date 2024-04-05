#include "PhysicsEngine.hpp"
#include "core/Logger.hpp"
#include "core/Timer.hpp"
#include "eventSystem/EventDispatcher.hpp"
#include "scene/Scene.hpp"
#include "components/TransformComponent.hpp"
#include "components/physics/SphereCollider.hpp"
#include "components/physics/CylinderCollider.hpp"
#include "mesh/CylinderMesh.hpp"
#include "components/physics/BoxColliderComponent.hpp"
#include <typeinfo>

#include "events/gameObjects/GameObjectDestroyEvent.hpp"
#include "events/physics/registry/AddColliderEvent.hpp"
#include "events/physics/registry/AddRigidBodyEvent.hpp"
#include "events/physics/registry/RemoveColliderEvent.hpp"
#include "events/physics/registry/RemoveRigidBodyEvent.hpp"

namespace TechEngine {
    PhysicsEngine::PhysicsEngine(Scene& scene) : scene(scene) {
        callback = new PhysicsCallback(scene, *this);

        EventDispatcher::getInstance().subscribe(GameObjectDestroyEvent::eventType, [this](Event* event) {
            std::string tag = ((GameObjectDestroyEvent*)event)->getGameObjectTag();
            removeActor(tag);
        });
        EventDispatcher::getInstance().subscribe(AddColliderEvent::eventType, [this](Event* event) {
            addCollider(((AddColliderEvent*)event)->getCollider());
        });
        EventDispatcher::getInstance().subscribe(RemoveColliderEvent::eventType, [this](Event* event) {
            removeCollider(((RemoveColliderEvent*)event)->getTag(), ((RemoveColliderEvent*)event)->getCollider());
        });

        EventDispatcher::getInstance().subscribe(AddRigidBodyEvent::eventType, [this](Event* event) {
            addRigidBody(((AddRigidBodyEvent*)event)->getRigidBody());
        });

        EventDispatcher::getInstance().subscribe(RemoveRigidBodyEvent::eventType, [this](Event* event) {
            removeRigidBody(((RemoveRigidBodyEvent*)event)->getRigidBody()->getGameObject()->getTag());
        });
    }

    PhysicsEngine::~PhysicsEngine() {
        pxScene->release();
        dispatcher->release();
        physics->release();
        pvd->release();
        foundation->release();
        delete callback;
    }

    void PhysicsEngine::init() {
        foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
        if (!foundation)
            TE_LOGGER_CRITICAL("PxCreateFoundation failed!");
        pvd = PxCreatePvd(*foundation);
        physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("localhost", 5425, 10);
        if (pvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL)) {
            TE_LOGGER_INFO("PVD connected!");
        } else {
            TE_LOGGER_ERROR("PVD connection failed!");
        }
        physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, physx::PxTolerancesScale(), true, pvd);
        if (!physics)
            TE_LOGGER_ERROR("PxCreatePhysics failed!");

        defaultMaterial = physics->createMaterial(0.5f, 0.5f, 0.0f);
        createScene();

        TE_LOGGER_INFO("Physics engine initialized!");
    }

    physx::PxFilterFlags CollisionFilterShader(
        physx::PxFilterObjectAttributes attributes0, physx::PxFilterData filterData0,
        physx::PxFilterObjectAttributes attributes1, physx::PxFilterData filterData1,
        physx::PxPairFlags& pairFlags, const void* constantBlock, physx::PxU32 constantBlockSize) {
        // Enable contact and trigger events for all pairs of interacting objects
        pairFlags = physx::PxPairFlag::eCONTACT_DEFAULT |
                    physx::PxPairFlag::eTRIGGER_DEFAULT |
                    physx::PxPairFlag::eNOTIFY_TOUCH_FOUND |
                    physx::PxPairFlag::eNOTIFY_TOUCH_LOST;

        return physx::PxFilterFlag::eDEFAULT;
    }

    void PhysicsEngine::createScene() {
        physx::PxSceneDesc sceneDesc(physics->getTolerancesScale());
        sceneDesc.gravity = physx::PxVec3(0.0f, -9.81f, 0.0f);
        dispatcher = physx::PxDefaultCpuDispatcherCreate(2);
        sceneDesc.cpuDispatcher = dispatcher;
        sceneDesc.filterShader = CollisionFilterShader;
        pxScene = physics->createScene(sceneDesc);
        if (!pxScene) {
            TE_LOGGER_CRITICAL("createScene failed!");
        }
        pxScene->setSimulationEventCallback(callback);
    }

    void PhysicsEngine::start() {
        if (!pxScene)
            TE_LOGGER_CRITICAL("createScene failed!");
        if (!rigidBodiesWithoutColliders.empty()) {
            for (const std::string& tag: rigidBodiesWithoutColliders) {
                GameObject* gameObject = scene.getGameObjectByTag(tag);
                TE_LOGGER_WARN("GameObject {0} has rigid body without collider!", gameObject->getName());
            }
        }
        for (auto& actor: actors) {
            GameObject* gameObject = scene.getGameObjectByTag(actor.first);
            TransformComponent* transformComponent = gameObject->getComponent<TransformComponent>();
            glm::vec3 position = transformComponent->getPosition();
            glm::quat rotation = glm::quat(glm::radians(transformComponent->getOrientation()));
            actor.second->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
        }
        running = true;
    }

    void PhysicsEngine::stop() {
        pxScene->release();
        createScene();
        running = false;
    }

    void PhysicsEngine::onFixedUpdate() {
        if (running) {
            updateActorPositions();
            pxScene->simulate(Timer::getInstance().getTPS());
            pxScene->fetchResults(true);
            updateGameObjectPositions();
        }
    }

    void PhysicsEngine::clear() {
        for (auto& actor: actors) {
            actor.second->release();
        }
        actors.clear();
        rigidBodiesWithoutColliders.clear();
    }

    void PhysicsEngine::updateActorPositions() {
        for (auto& actor: actors) {
            TransformComponent* transformComponent = static_cast<TransformComponent*>(actor.second->userData);
            glm::vec3 position = transformComponent->getWorldPosition();
            glm::quat rotation = glm::quat(glm::radians(transformComponent->getOrientation()));
            actor.second->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
        }
    }

    void PhysicsEngine::updateGameObjectPositions() {
        for (auto& actor: actors) {
            physx::PxTransform transform = actor.second->getGlobalPose();
            TransformComponent* transformComponent = static_cast<TransformComponent*>(actor.second->userData);
            transformComponent->translateToWorld(glm::vec3(transform.p.x, transform.p.y, transform.p.z));
            transformComponent->setRotation(glm::quat(transform.q.w, transform.q.x, transform.q.y, transform.q.z));
        }
    }

    void PhysicsEngine::addCollider(Collider* collider) {
        TransformComponent& t = collider->getTransform();
        glm::vec3 position = collider->getTransform().getWorldPosition();
        glm::quat rotation = glm::quat(glm::radians(t.getOrientation()));
        physx::PxTransform transform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w));
        physx::PxTransform transformOffset(physx::PxVec3(collider->getOffset().x, -collider->getOffset().y, collider->getOffset().z));
        physx::PxGeometry* geometry = createGeometry(t, collider);
        if (std::find(rigidBodiesWithoutColliders.begin(), rigidBodiesWithoutColliders.end(), collider->getGameObject()->getTag()) != rigidBodiesWithoutColliders.end()) {
            rigidBodiesWithoutColliders.remove(collider->getGameObject()->getTag());
        }
        if (actors.find(collider->getGameObject()->getTag()) != actors.end()) {
            updateActor(collider, transformOffset, geometry);
        } else {
            addActor(collider, transform, geometry, transformOffset);
        }
        delete geometry;
    }

    void PhysicsEngine::addRigidBody(RigidBody* rigidBody) {
        if (actors.find(rigidBody->getGameObject()->getTag()) == actors.end()) {
            rigidBodiesWithoutColliders.push_back(rigidBody->getGameObject()->getTag());
        } else {
            //removeActor(rigidBody->getGameObject()->getTag());
            for (std::pair<std::string, Component*> component: rigidBody->getGameObject()->getComponents()) {
                if (Collider* collider = dynamic_cast<Collider*>(component.second)) {
                    addCollider(collider);
                }
            }
        }
    }

    void PhysicsEngine::removeCollider(const std::string& gameObjectTag, Collider* collider) {
        if (actors.find(gameObjectTag) != actors.end()) {
            physx::PxRigidActor* actor = actors[gameObjectTag];
            detachCollider(collider);
            if (actor->getNbShapes() == 0) {
                if (collider->hasRigidBody()) {
                    rigidBodiesWithoutColliders.push_back(gameObjectTag);
                }
                actors[gameObjectTag]->release();
                actors.erase(gameObjectTag);
            }
        }
    }

    void PhysicsEngine::removeRigidBody(const std::string& tag) {
        if (std::find(rigidBodiesWithoutColliders.begin(), rigidBodiesWithoutColliders.end(), tag) != rigidBodiesWithoutColliders.end()) {
            rigidBodiesWithoutColliders.remove(tag);
        } else {
            GameObject* gameObject = scene.getGameObjectByTag(tag);
            if (gameObject == nullptr) {
                return;
            }
            for (std::pair<std::string, Component*> component: gameObject->getComponents()) {
                if (Collider* collider = dynamic_cast<Collider*>(component.second)) {
                    addCollider(collider);
                }
            }
        }
    }

    physx::PxGeometry* PhysicsEngine::createGeometry(TransformComponent& transform, Collider* collider) {
        if (BoxColliderComponent* boxCollider = dynamic_cast<BoxColliderComponent*>(collider)) {
            glm::vec3 size = transform.getScale() / glm::vec3(2, 2, 2) * boxCollider->getSize();
            return new physx::PxBoxGeometry(size.x, size.y, size.z);
        } else if (SphereCollider* sphereCollider = dynamic_cast<SphereCollider*>(collider)) {
            return new physx::PxSphereGeometry(sphereCollider->getRadius());
        } else if (CylinderCollider* cylinderCollider = dynamic_cast<CylinderCollider*>(collider)) {
            return createCylinderMesh(cylinderCollider, transform);
        } else {
            TE_LOGGER_CRITICAL("Collider type not supported!");
            return nullptr;
        }
    }

    physx::PxGeometry* PhysicsEngine::createCylinderMesh(CylinderCollider* cylinderCollider, TransformComponent& t) {
        CylinderMesh mesh = CylinderMesh();
        cylinderCollider->getHeight();
        cylinderCollider->getRadius();
        physx::PxConvexMeshDesc meshDesc;
        meshDesc.points.count = static_cast<physx::PxU32>(mesh.getVertices().size());
        meshDesc.points.stride = sizeof(physx::PxVec3);
        std::vector<physx::PxVec3> convexPoints = std::vector<physx::PxVec3>(mesh.getVertices().size());
        for (int i = 0; i < mesh.getVertices().size(); i++) {
            glm::vec3 scale = t.getScale();
            glm::vec3 position = mesh.getVertices()[i].getPosition() * scale;
            convexPoints[i] = physx::PxVec3(position.x * cylinderCollider->getRadius() * 2, position.y * cylinderCollider->getHeight(), position.z * cylinderCollider->getRadius() * 2);
        }
        meshDesc.points.data = convexPoints.data();
        meshDesc.flags = physx::PxConvexFlag::eCOMPUTE_CONVEX;

        meshDesc.indices.count = static_cast<physx::PxU32>(mesh.getIndices().size());
        meshDesc.indices.stride = sizeof(physx::PxU32);
        meshDesc.indices.data = mesh.getIndices().data();
        physx::PxTolerancesScale scale;
        physx::PxCookingParams params(scale);

        physx::PxDefaultMemoryOutputStream buf;
        physx::PxConvexMeshCookingResult::Enum result;
        physx::PxConvexMesh* convexMesh;
        if (PxCookConvexMesh(params, meshDesc, buf, &result)) {
            physx::PxDefaultMemoryInputData input(buf.getData(), buf.getSize());
            convexMesh = physics->createConvexMesh(input);
        } else {
            TE_LOGGER_CRITICAL("Cooking cylinder failed!");
            return nullptr;
        }
        physx::PxConvexMeshGeometry* convexMeshGeometry = new physx::PxConvexMeshGeometry(convexMesh);
        return convexMeshGeometry;
    }

    void PhysicsEngine::addActor(Collider* collider, const physx::PxTransform& transform, const physx::PxGeometry* geometry, const physx::PxTransform& transformOffset) {
        physx::PxRigidActor* actor;
        physx::PxMaterial* material = physics->createMaterial(defaultMaterial->getStaticFriction(), defaultMaterial->getDynamicFriction(), defaultMaterial->getRestitution());
        if (collider->hasRigidBody()) {
            actor = PxCreateDynamic(*physics, transform, *geometry, *material, 10.0f, transformOffset);
        } else {
            actor = PxCreateStatic(*physics, transform, *geometry, *material, transformOffset);
        }
        actor->userData = collider->getGameObject()->getComponent<TransformComponent>();
        actors[collider->getGameObject()->getTag()] = actor;
        if (geometry->getType() == physx::PxGeometryType::eCONVEXMESH) {
            physx::PxConvexMeshGeometry* convexMeshGeometry = (physx::PxConvexMeshGeometry*)geometry;
            convexMeshGeometry->convexMesh->release();
        }
        pxScene->addActor(*actor);
    }

    void PhysicsEngine::updateActor(Collider* collider, const physx::PxTransform& transformOffset, const physx::PxGeometry* geometry) {
        detachCollider(collider);
        physx::PxRigidActor* actor = actors[collider->getGameObject()->getTag()];
        physx::PxShape* shape = physics->createShape(*geometry, *defaultMaterial);
        shape->setLocalPose(transformOffset);
        actor->attachShape(*shape);
        if (geometry->getType() == physx::PxGeometryType::eCONVEXMESH) {
            auto* convexMeshGeometry = (physx::PxConvexMeshGeometry*)geometry;
            convexMeshGeometry->convexMesh->release();
        }
    }

    void PhysicsEngine::removeActor(const std::string& tag) {
        if (actors.find(tag) != actors.end()) {
            pxScene->removeActor(*actors[tag]);
        }
    }

    void PhysicsEngine::updateMaterial(Collider* collider) {
        if (actors.find(collider->getGameObject()->getTag()) != actors.end()) {
            physx::PxRigidActor* actor = actors[collider->getGameObject()->getTag()];
            int shapesNumber = actor->getNbShapes();
            physx::PxShape** shapes = new physx::PxShape*[shapesNumber];
            actor->getShapes(shapes, shapesNumber);
            for (int i = 0; i < shapesNumber; i++) {
                shapes[i]->setMaterials(&defaultMaterial, 1);
            }
        }
    }

    std::pair<int, physx::PxShape**> PhysicsEngine::getShapes(GameObject* gameObject) {
        physx::PxRigidActor* actor = actors[gameObject->getTag()];
        int shapesNumber = actor->getNbShapes();
        physx::PxShape** shapes = new physx::PxShape*[shapesNumber];
        actor->getShapes(shapes, shapesNumber);
        return std::make_pair(shapesNumber, shapes);
    }

    void PhysicsEngine::detachCollider(Collider* collider) {
        physx::PxRigidActor* actor = actors[collider->getGameObject()->getTag()];
        std::pair<int, physx::PxShape**> shapes = getShapes(collider->getGameObject());
        for (int i = 0; i < shapes.first; i++) {
            if (shapes.second[i]->getGeometry().getType() == physx::PxGeometryType::eBOX && typeid(*collider) == typeid(BoxColliderComponent)) {
                actor->detachShape(*shapes.second[i]);
                break;
            } else if (shapes.second[i]->getGeometry().getType() == physx::PxGeometryType::eSPHERE && typeid(*collider) == typeid(SphereCollider)) {
                actor->detachShape(*shapes.second[i]);
                break;
            } else if (shapes.second[i]->getGeometry().getType() == physx::PxGeometryType::eCONVEXMESH && typeid(*collider) == typeid(CylinderCollider)) {
                actor->detachShape(*shapes.second[i]);
                break;
            }
        }
    }
}
